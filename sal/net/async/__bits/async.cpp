#include <sal/net/async/__bits/async.hpp>
#include <sal/error.hpp>

#if __sal_os_linux || __sal_os_macos
  #include <cstring>
  #include <unistd.h>
#endif

#if __sal_os_linux
  #include <sys/epoll.h>
#elif __sal_os_macos
  #include <sys/event.h>
  #include <sys/time.h>
  #include <sys/types.h>
#endif


__sal_begin


namespace net::async::__bits {


#if __sal_os_windows //{{{1


namespace {

using net::__bits::winsock;
constexpr DWORD acceptex_address_size = sizeof(sockaddr_storage) + 16;


inline WSABUF make_buf (io_t *io) noexcept
{
  WSABUF result;
  result.buf = reinterpret_cast<CHAR *>(io->begin);
  result.len = static_cast<ULONG>(io->end - io->begin);
  return result;
}


inline void io_result_handle (io_t *io, int result) noexcept
{
  if (result == 0)
  {
    io->status.clear();
    io->owner.enqueue(io);
    return;
  }

  switch (auto e = ::WSAGetLastError())
  {
    case WSA_IO_PENDING:
      return;

    case WSAESHUTDOWN:
      io->status = std::make_error_code(std::errc::broken_pipe);
      break;

    default:
      io->status.assign(e, std::system_category());
      break;
  }

  io->owner.enqueue(io);
}


bool finish (io_t *io) noexcept
{
  DWORD transferred;
  auto result = ::WSAGetOverlappedResult(
    io->current_owner->socket.handle,
    reinterpret_cast<WSAOVERLAPPED *>(io),
    &transferred,
    false,
    io->flags
  );
  *io->transferred = transferred;

  if (result)
  {
    return true;
  }

  switch (auto e = ::WSAGetLastError())
  {
    case WSAENOTSOCK:
      io->status = std::make_error_code(std::errc::operation_canceled);
      break;

    case WSAEADDRNOTAVAIL:
      io->status = std::make_error_code(std::errc::address_family_not_supported);
      break;

    default:
      io->status.assign(e, std::system_category());
      break;
  }
  return false;
}


bool finish_receive (io_t *io) noexcept
{
  if (finish(io))
  {
    if (*io->transferred > 0)
    {
      io->status.clear();
      return true;
    }
    io->status = std::make_error_code(std::errc::broken_pipe);
  }
  return false;
}


bool finish_accept (io_t *io) noexcept
{
  if (finish(io))
  {
    auto result = ::setsockopt(
      *io->pending.accept.socket_handle,
      SOL_SOCKET,
      SO_UPDATE_ACCEPT_CONTEXT,
      reinterpret_cast<char *>(&io->current_owner->socket.handle),
      sizeof(io->current_owner->socket.handle)
    );
    if (result != SOCKET_ERROR)
    {
      io->status.clear();
      return true;
    }
    io->status.assign(::WSAGetLastError(), std::system_category());
  }
  return false;
}


bool finish_connect (io_t *io) noexcept
{
  if (finish(io))
  {
    auto result = ::setsockopt(
      io->current_owner->socket.handle,
      SOL_SOCKET,
      SO_UPDATE_CONNECT_CONTEXT,
      nullptr,
      0
    );
    if (result != SOCKET_ERROR)
    {
      io->status.clear();
      return true;
    }
    io->status.assign(::WSAGetLastError(), std::system_category());
  }
  return false;
}


} // namespace


service_t::service_t ()
  : iocp(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0))
{
  if (iocp == INVALID_HANDLE_VALUE)
  {
    std::error_code error;
    error.assign(::GetLastError(), std::system_category());
    throw_system_error(error, "CreateIoCompletionPort");
  }
}


service_t::~service_t () noexcept
{
  if (iocp != INVALID_HANDLE_VALUE)
  {
    (void)::CloseHandle(iocp);
  }
}


bool service_t::wait_for_more (const std::chrono::milliseconds &timeout,
  std::error_code &error) noexcept
{
  constexpr DWORD max_events = 256;
  ::OVERLAPPED_ENTRY events[max_events];
  ULONG events_count;

  auto succeeded = ::GetQueuedCompletionStatusEx(
    iocp,
    events,
    max_events,
    &events_count,
    static_cast<DWORD>(timeout.count()),
    false
  );

  if (succeeded)
  {
    for (auto event = &events[0];  event != &events[0] + events_count;  ++event)
    {
      auto io = reinterpret_cast<io_t *>(event->lpOverlapped);
      (*io->on_finish)(io);
      enqueue(io);
    }
    return true;
  }

  auto e = ::GetLastError();
  if (e == WAIT_TIMEOUT)
  {
    error.clear();
  }
  else
  {
    error.assign(e, std::system_category());
  }
  return false;
}


handler_t::handler_t (service_ptr service,
    socket_t &socket,
    std::error_code &error) noexcept
  : service(service)
  , socket(socket.handle)
{
  auto result = ::CreateIoCompletionPort(
    reinterpret_cast<HANDLE>(socket.handle),
    service->iocp,
    0,
    0
  );
  if (!result)
  {
    error.assign(::GetLastError(), std::system_category());
  }
}


handler_t::~handler_t () noexcept
{
  socket.handle = socket.invalid;
}


void handler_t::start_receive_from (io_t *io,
  void *remote_endpoint,
  size_t remote_endpoint_capacity,
  size_t *transferred,
  message_flags_t *flags) noexcept
{
  io->current_owner = this;
  io->on_finish = finish;
  io->transferred = transferred;
  io->flags = flags;

  io->pending.receive_from.remote_endpoint_capacity =
    static_cast<INT>(remote_endpoint_capacity);

  DWORD received;
  auto buf = make_buf(io);
  auto result = ::WSARecvFrom(
    socket.handle,
    &buf,
    1,
    &received,
    io->flags,
    static_cast<sockaddr *>(remote_endpoint),
    &io->pending.receive_from.remote_endpoint_capacity,
    &io->overlapped,
    nullptr
  );

  if (result == 0)
  {
    *io->transferred = received;
  }

  io_result_handle(io, result);
}


void handler_t::start_receive (io_t *io,
  size_t *transferred,
  message_flags_t *flags) noexcept
{
  io->current_owner = this;
  io->on_finish = finish_receive;
  io->transferred = transferred;
  io->flags = flags;

  DWORD received;
  auto buf = make_buf(io);
  auto result = ::WSARecv(
    socket.handle,
    &buf,
    1,
    &received,
    io->flags,
    &io->overlapped,
    nullptr
  );

  if (result == 0)
  {
    *io->transferred = received;
    if (*io->transferred == 0)
    {
      ::WSASetLastError(WSAESHUTDOWN);
      result = SOCKET_ERROR;
    }
  }

  io_result_handle(io, result);
}


void handler_t::start_send_to (io_t *io,
  const void *remote_endpoint,
  size_t remote_endpoint_size,
  size_t *transferred,
  message_flags_t flags) noexcept
{
  io->current_owner = this;
  io->on_finish = finish;
  io->transferred = transferred;
  io->flags = &io->pending.send_to.flags;

  DWORD sent;
  auto buf = make_buf(io);
  auto result = ::WSASendTo(
    socket.handle,
    &buf,
    1,
    &sent,
    flags,
    static_cast<const sockaddr *>(remote_endpoint),
    static_cast<int>(remote_endpoint_size),
    &io->overlapped,
    nullptr
  );

  if (result == 0)
  {
    *io->transferred = sent;
  }

  io_result_handle(io, result);
}


void handler_t::start_send (io_t *io,
  size_t *transferred,
  message_flags_t flags) noexcept
{
  io->current_owner = this;
  io->on_finish = finish;
  io->transferred = transferred;
  io->flags = &io->pending.send.flags;

  DWORD sent;
  auto buf = make_buf(io);
  auto result = ::WSASend(
    socket.handle,
    &buf,
    1,
    &sent,
    flags,
    &io->overlapped,
    nullptr
  );

  if (result == 0)
  {
    *io->transferred = sent;
  }

  io_result_handle(io, result);
}


void handler_t::start_accept (io_t *io,
  int family,
  socket_t::handle_t *socket_handle) noexcept
{
  io->current_owner = this;
  io->on_finish = finish_accept;
  io->transferred = &io->pending.accept.unused_transferred;
  io->flags = &io->pending.accept.unused_flags;
  io->pending.accept.socket_handle = socket_handle;

  socket_t new_socket;
  new_socket.open(family, SOCK_STREAM, IPPROTO_TCP, io->status);
  *io->pending.accept.socket_handle = new_socket.handle;

  if (io->status)
  {
    service->enqueue(io);
    return;
  }

  new_socket.handle = socket_t::invalid;

  auto success = winsock.AcceptEx(
    socket.handle,
    *io->pending.accept.socket_handle,
    io->data,
    0,
    0,
    acceptex_address_size,
    nullptr,
    &io->overlapped
  );

  int result = SOCKET_ERROR;
  if (success)
  {
    result = ::setsockopt(
      *io->pending.accept.socket_handle,
      SOL_SOCKET,
      SO_UPDATE_ACCEPT_CONTEXT,
      reinterpret_cast<char *>(&io->current_owner->socket.handle),
      sizeof(io->current_owner->socket.handle)
    );
  }

  io_result_handle(io, result);
}


void handler_t::start_connect (io_t *io,
  const void *remote_endpoint,
  size_t remote_endpoint_size) noexcept
{
  io->current_owner = this;
  io->on_finish = finish_connect;
  io->transferred = &io->pending.connect.unused_transferred;
  io->flags = &io->pending.connect.unused_flags;

  auto success = winsock.ConnectEx(
    socket.handle,
    static_cast<const sockaddr *>(remote_endpoint),
    static_cast<int>(remote_endpoint_size),
    nullptr,
    0,
    nullptr,
    &io->overlapped
  );

  auto result = SOCKET_ERROR;
  if (success)
  {
    result = ::setsockopt(
      io->current_owner->socket.handle,
      SOL_SOCKET,
      SO_UPDATE_CONNECT_CONTEXT,
      nullptr,
      0
    );
  }

  io_result_handle(io, result);
}


#elif __sal_os_linux //{{{1


namespace {


inline int make_queue () noexcept
{
  return ::epoll_create1(0);
}


void register_handler (handler_t &handler, std::error_code &error)
  noexcept
{
  struct ::epoll_event change;
  change.events = handler.await_events = EPOLLET;
  change.data.ptr = &handler;

  auto result = ::epoll_ctl(
    handler.service->queue,
    EPOLL_CTL_ADD,
    handler.socket.handle,
    &change
  );

  if (result > -1)
  {
    error.clear();
  }
  else
  {
    error.assign(errno, std::generic_category());
  }
}


bool await_io (handler_t &handler, uint32_t await_events, std::error_code &error)
  noexcept
{
  if (handler.await_events != await_events)
  {
    struct ::epoll_event change;
    change.events = handler.await_events = await_events;
    change.data.ptr = &handler;

    auto result = ::epoll_ctl(
      handler.service->queue,
      EPOLL_CTL_MOD,
      handler.socket.handle,
      &change
    );

    if (result < 0)
    {
      error.assign(errno, std::generic_category());
      return true;
    }
  }

  return false;
}


inline bool await_read (io_t *io) noexcept
{
  if (io->status == std::errc::operation_would_block)
  {
    auto &handler = *io->current_owner;
    return await_io(handler, handler.await_events | EPOLLIN, io->status);
  }
  return true;
}


inline bool await_write (io_t *io) noexcept
{
  if (io->status == std::errc::operation_would_block)
  {
    auto &handler = *io->current_owner;
    return await_io(handler, handler.await_events | EPOLLOUT, io->status);
  }
  return true;
}


bool drain (handler_t &handler, handler_t::pending_t &pending, uint32_t events)
  noexcept
{
  std::lock_guard lock(pending.mutex);

  while (auto io = static_cast<io_t *>(pending.list.head()))
  {
    if ((*io->on_finish)(io, 0, events))
    {
      (void)pending.list.try_pop();
      handler.service->enqueue(io);
    }
    else
    {
      return false;
    }
  }

  return true;
}


void drain (struct ::epoll_event &event) noexcept
{
  auto &handler = *static_cast<handler_t *>(event.data.ptr);
  auto await_events = handler.await_events;

  if ((event.events & EPOLLIN)
    && drain(handler, handler.pending_read, event.events))
  {
    await_events &= ~EPOLLIN;
  }

  if ((event.events & EPOLLOUT)
    && drain(handler, handler.pending_write, event.events))
  {
    await_events &= ~EPOLLOUT;
  }

  if (handler.await_events != await_events)
  {
    std::error_code ignore_error;
    await_io(handler, await_events, ignore_error);
  }
}


inline bool complete_connection (io_t *io, uint16_t /**/, uint32_t events)
  noexcept
{
  if (events & EPOLLERR)
  {
    int data;
    socklen_t size = sizeof(data);
    auto result = ::getsockopt(
      io->current_owner->socket.handle,
      SOL_SOCKET,
      SO_ERROR,
      &data,
      &size
    );
    if (result == 0)
    {
      io->status.assign(data, std::generic_category());
    }
    else
    {
      io->status.assign(errno, std::generic_category());
    }
  }
  else
  {
    io->status.clear();
  }

  return true;
}


} // namespace


#elif __sal_os_macos //{{{1


namespace {


inline ::timespec *to_timespec (::timespec *ts,
  const std::chrono::milliseconds &timeout) noexcept
{
  using namespace std::chrono;

  if (timeout != (milliseconds::max)())
  {
    auto s = duration_cast<seconds>(timeout);
    ts->tv_nsec = duration_cast<nanoseconds>(timeout - s).count();
    ts->tv_sec = s.count();
    return ts;
  }
  return nullptr;
}


inline int make_queue () noexcept
{
  return ::kqueue();
}


inline void register_handler (handler_t &, std::error_code &error) noexcept
{
  error.clear();
}


bool await_io (io_t *io, int16_t filter, uint16_t flags) noexcept
{
  if (io->status != std::errc::operation_would_block)
  {
    return true;
  }

  auto &handler = *io->current_owner;

  struct ::kevent change;
  EV_SET(&change,
    handler.socket.handle,
    filter,
    flags,
    0,
    0,
    &handler
  );

  auto result = ::kevent(
    handler.service->queue,
    &change,
    1,
    nullptr,
    0,
    nullptr
  );

  if (result > -1)
  {
    return false;
  }

  io->status.assign(errno, std::generic_category());
  return true;
}


inline bool await_read (io_t *io) noexcept
{
  return await_io(io, EVFILT_READ, EV_ADD | EV_CLEAR);
}


inline bool await_write (io_t *io) noexcept
{
  return await_io(io, EVFILT_WRITE, EV_ADD | EV_CLEAR);
}


void drain (const struct ::kevent &event) noexcept
{
  handler_t::pending_t *pending_io{};

  auto &handler = *static_cast<handler_t *>(event.udata);
  if (event.filter == EVFILT_READ)
  {
    pending_io = &handler.pending_read;
  }
  else if (event.filter == EVFILT_WRITE)
  {
    pending_io = &handler.pending_write;
  }

  std::lock_guard lock(pending_io->mutex);

  while (auto io = static_cast<io_t *>(pending_io->list.head()))
  {
    if ((*io->on_finish)(io, event.flags, event.fflags))
    {
      (void)pending_io->list.try_pop();
      handler.service->enqueue(io);
    }
    else
    {
      return;
    }
  }
}


inline bool complete_connection (io_t *io, uint16_t flags, uint32_t fflags)
  noexcept
{
  if (flags & EV_EOF)
  {
    io->status.assign(fflags, std::generic_category());
  }
  else
  {
    io->status.clear();
  }
  return true;
}


} // namespace


#endif //}}}1


#if __sal_os_linux || __sal_os_macos //{{{1


service_t::service_t ()
  : queue(make_queue())
{
  if (queue < 0)
  {
    std::error_code error;
    error.assign(errno, std::generic_category());
    throw_system_error(error, "async::service::make_queue");
  }
}


service_t::~service_t () noexcept
{
  (void)::close(queue);
}


bool service_t::wait_for_more (const std::chrono::milliseconds &timeout,
  std::error_code &error) noexcept
{
  constexpr size_t max_events = 256;

#if __sal_os_linux

  struct ::epoll_event events[max_events];
  auto events_count = ::epoll_wait(
    queue,
    &events[0],
    max_events,
    timeout == timeout.max() ? -1 : timeout.count()
  );

#elif __sal_os_macos

  ::timespec ts, *timeout_p = to_timespec(&ts, timeout);
  struct ::kevent events[max_events];
  auto events_count = ::kevent(
    queue,
    nullptr,
    0,
    &events[0],
    max_events,
    timeout_p
  );

#endif

  if (events_count > -1)
  {
    for (auto event = &events[0];  event != &events[0] + events_count;  ++event)
    {
      drain(*event);
    }
    return events_count > 0;
  }

  error.assign(errno, std::generic_category());
  return false;
}


handler_t::handler_t (service_ptr service,
    socket_t &socket,
    std::error_code &error) noexcept
  : service(service)
  , socket(socket.handle)
{
  register_handler(*this, error);
}


handler_t::~handler_t () noexcept
{
  socket.handle = socket.invalid;

  auto cancel = [&](auto &pending)
  {
    while (auto io = pending.list.try_pop())
    {
      io->status = std::make_error_code(std::errc::operation_canceled);
      io->current_owner = nullptr;
      service->enqueue(static_cast<io_t *>(io));
    }
  };
  cancel(pending_read);
  cancel(pending_write);
}


namespace {


inline void start (io_t *io, handler_t::pending_t &pending) noexcept
{
  std::lock_guard lock(pending.mutex);
  if (pending.list.empty() && (*io->on_finish)(io, 0, 0))
  {
    io->owner.enqueue(io);
  }
  else
  {
    pending.list.push(io);
  }
}


bool finish_receive_from (io_t *io, uint16_t, uint32_t) noexcept
{
  *io->transferred = io->current_owner->socket.receive_from(
    io->begin,
    io->end - io->begin,
    io->pending.receive_from.remote_endpoint,
    &io->pending.receive_from.remote_endpoint_capacity,
    *io->flags,
    io->status
  );
  return await_read(io);
}


bool finish_receive (io_t *io, uint16_t, uint32_t) noexcept
{
  *io->transferred = io->current_owner->socket.receive(
    io->begin,
    io->end - io->begin,
    *io->flags,
    io->status
  );
  return await_read(io);
}


bool finish_send_to (io_t *io, uint16_t, uint32_t) noexcept
{
  *io->transferred = io->current_owner->socket.send_to(
    io->begin,
    io->end - io->begin,
    &io->pending.send_to.remote_endpoint,
    io->pending.send_to.remote_endpoint_size,
    io->pending.send_to.flags,
    io->status
  );
  return await_write(io);
}


bool finish_send (io_t *io, uint16_t, uint32_t) noexcept
{
  *io->transferred = io->current_owner->socket.send(
    io->begin,
    io->end - io->begin,
    io->pending.send_to.flags,
    io->status
  );
  return await_write(io);
}


bool finish_accept (io_t *io, uint16_t, uint32_t) noexcept
{
  *io->pending.accept.socket_handle = io->current_owner->socket.accept(
    nullptr,
    nullptr,
    false,
    io->status
  );
  return await_read(io);
}


bool finish_connect (io_t *io, uint16_t flags, uint32_t fflags) noexcept
{
  if (io->status == std::errc::operation_in_progress)
  {
    // just started socket.connect() in start_connect()
    // wait socket to become writable
    io->status = std::make_error_code(std::errc::operation_would_block);
    return await_write(io);
  }
  else if (io->status != std::errc::operation_would_block)
  {
    // blocking socket, we know result immediately
    return true;
  }

  // final state (connected, rejected, etc)
  return complete_connection(io, flags, fflags);
}


} // namespace


void handler_t::start_receive_from (io_t *io,
  void *remote_endpoint,
  size_t remote_endpoint_capacity,
  size_t *transferred,
  message_flags_t *flags) noexcept
{
  io->current_owner = this;
  io->on_finish = finish_receive_from;
  io->transferred = transferred;
  io->flags = flags;

  *io->flags |= MSG_DONTWAIT;
  io->pending.receive_from.remote_endpoint = remote_endpoint;
  io->pending.receive_from.remote_endpoint_capacity = remote_endpoint_capacity;

  start(io, pending_read);
}


void handler_t::start_receive (io_t *io,
  size_t *transferred,
  message_flags_t *flags) noexcept
{
  io->current_owner = this;
  io->on_finish = finish_receive;
  io->transferred = transferred;
  io->flags = flags;

  *io->flags |= MSG_DONTWAIT;

  start(io, pending_read);
}


void handler_t::start_send_to (io_t *io,
  const void *remote_endpoint,
  size_t remote_endpoint_size,
  size_t *transferred,
  message_flags_t flags) noexcept
{
  io->current_owner = this;
  io->on_finish = finish_send_to;
  io->transferred = transferred;

  io->pending.send_to.flags = flags | MSG_DONTWAIT;

  io->pending.send_to.remote_endpoint_size = remote_endpoint_size;
  memcpy(
    &io->pending.send_to.remote_endpoint,
    remote_endpoint,
    remote_endpoint_size
  );

  start(io, pending_write);
}


void handler_t::start_send (io_t *io,
  size_t *transferred,
  message_flags_t flags) noexcept
{
  io->current_owner = this;
  io->on_finish = finish_send;
  io->transferred = transferred;

  io->pending.send.flags = flags | MSG_DONTWAIT;

  start(io, pending_write);
}


void handler_t::start_accept (io_t *io,
  int /*family*/,
  socket_t::handle_t *socket_handle) noexcept
{
  io->current_owner = this;
  io->on_finish = finish_accept;

  io->pending.accept.socket_handle = socket_handle;

  start(io, pending_read);
}


void handler_t::start_connect (io_t *io,
  const void *remote_endpoint,
  size_t remote_endpoint_size) noexcept
{
  io->current_owner = this;
  io->on_finish = finish_connect;

  socket.connect(remote_endpoint, remote_endpoint_size, io->status);

  start(io, pending_write);
}


#endif //}}}1


io_t *service_t::make_io ()
{
  std::lock_guard lock(io_pool_mutex);

  auto io = free_list.try_pop();
  if (!io)
  {
    auto batch_size = 16 * (1ULL << io_pool.size());
    auto it = io_pool.emplace_back(new std::byte[batch_size * sizeof(io_t)]).get();
    io_pool_size += batch_size;

    while (batch_size--)
    {
      free_list.push(new(it) io_t(*this));
      it += sizeof(io_t);
    }

    io = free_list.try_pop();
  }

  io->current_owner = nullptr;
  return static_cast<io_t *>(io);
}


} // namespace net::async::__bits


__sal_end
