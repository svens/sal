#include <sal/__bits/platform_sdk.hpp>
#include <sal/net/__bits/async_socket.hpp>
#include <sal/net/error.hpp>
#include <sal/net/fwd.hpp>
#include <mutex>

#if __sal_os_windows
  #include <mswsock.h>
#elif __sal_os_darwin || __sal_os_linux
  #include <sal/spinlock.hpp>
  #include <sys/time.h>
  #include <unistd.h>
#endif


__sal_begin


namespace net { namespace __bits {


#if __sal_os_windows


namespace {


LPFN_CONNECTEX ConnectEx = nullptr;
LPFN_ACCEPTEX AcceptEx = nullptr;
LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockaddrs = nullptr;

constexpr DWORD acceptex_address_size = sizeof(sockaddr_storage) + 16;


template <typename Function>
void load_fn (Function *fn, GUID id, SOCKET socket, std::error_code &error)
  noexcept
{
  if (!error)
  {
    DWORD bytes;
    auto result = ::WSAIoctl(socket,
      SIO_GET_EXTENSION_FUNCTION_POINTER,
      &id, sizeof(id),
      fn, sizeof(fn),
      &bytes,
      nullptr,
      nullptr
    );
    if (result == SOCKET_ERROR)
    {
      error.assign(::WSAGetLastError(), std::system_category());
    }
  }
}


void internal_setup (std::error_code &error) noexcept
{
  if (!error)
  {
    error = sal::net::init();
  }

  if (!error)
  {
    auto socket = ::socket(AF_INET, SOCK_STREAM, 0);
    load_fn(&ConnectEx, WSAID_CONNECTEX, socket, error);
    load_fn(&AcceptEx, WSAID_ACCEPTEX, socket, error);
    load_fn(&GetAcceptExSockaddrs, WSAID_GETACCEPTEXSOCKADDRS, socket, error);
    ::closesocket(socket);
  }
}


} // namespace


void io_buf_t::io_result (int result) noexcept
{
  if (result == 0)
  {
    // completed immediately, caller still owns data
    if (transferred || request_id != async_receive_t::type_id())
    {
      error.clear();
    }
    else
    {
      // in case of receive, 0B means socket is closed
      error = make_error_code(std::errc::broken_pipe);
    }
    context->completed.push(this);
    return;
  }

  auto e = ::WSAGetLastError();
  if (e == WSA_IO_PENDING)
  {
    // pending, OS owns data
    return;
  }

  // failed, caller owns data
  else if (e == WSAESHUTDOWN)
  {
    error = make_error_code(std::errc::broken_pipe);
  }
  else if (e == WSAENOTSOCK)
  {
    error.assign(WSAEBADF, std::system_category());
  }
  else
  {
    error.assign(e, std::system_category());
  }
  context->completed.push(this);
}


void async_receive_t::start (socket_t &socket, message_flags_t flags) noexcept
{
  DWORD flags_ = flags;
  auto buf = to_buf();
  io_result(
    ::WSARecv(socket.handle,
      &buf, 1,
      &transferred,
      &flags_,
      this,
      nullptr
    )
  );
}


void async_receive_from_t::start (socket_t &socket, message_flags_t flags)
  noexcept
{
  address_size = sizeof(address);

  DWORD flags_ = flags;
  auto buf = to_buf();
  io_result(
    ::WSARecvFrom(socket.handle,
      &buf, 1,
      &transferred,
      &flags_,
      reinterpret_cast<sockaddr *>(&address),
      &address_size,
      this,
      nullptr
    )
  );
}


void async_send_to_t::start (socket_t &socket,
  const void *address, size_t address_size,
  message_flags_t flags) noexcept
{
  auto buf = to_buf();
  io_result(
    ::WSASendTo(socket.handle,
      &buf, 1,
      &transferred,
      flags,
      static_cast<const sockaddr *>(address),
      static_cast<int>(address_size),
      this,
      nullptr
    )
  );
}


void async_send_t::start (socket_t &socket, message_flags_t flags) noexcept
{
  auto buf = to_buf();
  io_result(
    ::WSASend(socket.handle,
      &buf, 1,
      &transferred,
      flags,
      this,
      nullptr
    )
  );
}


void async_connect_t::start (socket_t &socket,
  const void *address, size_t address_size) noexcept
{
  finished = false;
  handle = socket.handle;
  auto result = (*ConnectEx)(handle,
    static_cast<const sockaddr *>(address),
    static_cast<int>(address_size),
    nullptr, 0, nullptr,
    this
  );

  if (result == TRUE)
  {
    // immediate completion, caller still owns op
    error.clear();
    context->completed.push(this);
    return;
  }

  auto e = ::WSAGetLastError();
  if (e == WSA_IO_PENDING)
  {
    // pending, OS now owns op
    return;
  }

  // failed immediately, caller owns op
  error.assign(e, std::system_category());
  context->completed.push(this);
}


void async_connect_t::finish (std::error_code &result) noexcept
{
  switch (error.value())
  {
    case 0:
      if (!finished)
      {
        ::setsockopt(handle,
          SOL_SOCKET,
          SO_UPDATE_CONNECT_CONTEXT,
          nullptr,
          0
        );
        finished = true;
      }
      result.clear();
      return;

    case ERROR_INVALID_NETNAME:
      result = std::make_error_code(std::errc::address_not_available);
      return;

    case ERROR_CONNECTION_REFUSED:
      result = std::make_error_code(std::errc::connection_refused);
      return;

    default:
      result = error;
  }
}


void async_accept_t::start (socket_t &socket, int family) noexcept
{
  socket_t new_socket;
  new_socket.open(family, SOCK_STREAM, IPPROTO_TCP, error);
  if (error)
  {
    context->completed.push(this);
    return;
  }

  finished = false;
  acceptor = socket.handle;
  accepted = new_socket.handle;
  auto result = (*AcceptEx)(acceptor,
    accepted,
    begin,
    0,
    acceptex_address_size,
    acceptex_address_size,
    &transferred,
    this
  );
  if (result == TRUE)
  {
    error.clear();
    context->completed.push(this);
    return;
  }

  auto e = ::WSAGetLastError();
  if (e == ERROR_IO_PENDING)
  {
    return;
  }

  error.assign(e, std::system_category());
  context->completed.push(this);
}


void async_accept_t::finish (std::error_code &result) noexcept
{
  switch (error.value())
  {
    case 0:
      if (!finished)
      {
        int local_address_size, remote_address_size;
        (*GetAcceptExSockaddrs)(begin,
          0,
          acceptex_address_size,
          acceptex_address_size,
          reinterpret_cast<sockaddr **>(&local_address),
          &local_address_size,
          reinterpret_cast<sockaddr **>(&remote_address),
          &remote_address_size
        );

        ::setsockopt(accepted,
          SOL_SOCKET,
          SO_UPDATE_ACCEPT_CONTEXT,
          reinterpret_cast<char *>(&acceptor),
          sizeof(acceptor)
        );

        finished = true;
      }
      result.clear();
      break;

    case WSAENOTSOCK:
      result = std::make_error_code(std::errc::bad_file_descriptor);
      break;

    default:
      result = error;
      break;
  }
}


io_service_t::io_service_t (std::error_code &error) noexcept
  : iocp(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0))
{
  if (iocp)
  {
    error.clear();
    static std::once_flag flag;
    std::call_once(flag, &internal_setup, std::ref(error));
  }
  else
  {
    error.assign(::GetLastError(), std::system_category());
  }
}


io_service_t::~io_service_t () noexcept
{
  ::CloseHandle(iocp);
}


void io_service_t::associate (socket_t &socket, std::error_code &error)
  noexcept
{
  if (socket.associated)
  {
    error = make_error_code(socket_errc_t::already_associated);
    return;
  }

  auto result = ::CreateIoCompletionPort(
    reinterpret_cast<HANDLE>(socket.handle),
    iocp,
    0,
    0
  );

  if (result)
  {
    socket.associated = true;
  }
  else
  {
    error.assign(::GetLastError(), std::system_category());
  }
}


io_buf_t *io_context_t::try_get () noexcept
{
  if (completion != last_completion)
  {
    auto &entry = *completion++;
    auto *io_buf = static_cast<io_buf_t *>(entry.lpOverlapped);

    auto status = static_cast<NTSTATUS>(io_buf->Internal);
    if (NT_SUCCESS(status))
    {
      io_buf->transferred = entry.dwNumberOfBytesTransferred;
      if (!io_buf->transferred
        && io_buf->request_id == async_receive_t::type_id())
      {
        io_buf->error = make_error_code(std::errc::broken_pipe);
      }
    }
    else
    {
      if (status == STATUS_BUFFER_OVERFLOW)
      {
        io_buf->error.assign(WSAEMSGSIZE, std::system_category());
      }
      else
      {
        io_buf->error.assign(::RtlNtStatusToDosError(status),
          std::system_category()
        );
      }
      io_buf->transferred = 0;
    }

    io_buf->context = this;
    return io_buf;
  }
  return completed.try_pop();
}


io_buf_t *io_context_t::get (const std::chrono::milliseconds &timeout,
  std::error_code &error) noexcept
{
  error.clear();

  if (auto io_buf = try_get())
  {
    return io_buf;
  }

  completion = completions.begin();
  ULONG completion_count;
  auto succeeded = ::GetQueuedCompletionStatusEx(io_service.iocp,
    completions.data(), max_events_per_wait, &completion_count,
    static_cast<DWORD>(timeout.count()),
    false
  );
  if (succeeded)
  {
    last_completion = completion + completion_count;
    return try_get();
  }

  auto e = ::GetLastError();
  if (e != WAIT_TIMEOUT)
  {
    error.assign(e, std::system_category());
  }

  last_completion = completion;
  return nullptr;
}


#elif __sal_os_darwin || __sal_os_linux


struct async_worker_t
{
  socket_t &socket;
  io_service_t &io_service;

  using mutex_t = spinlock_t;
  using lock_t = std::lock_guard<mutex_t>;

  mutex_t receive_mutex{};
  io_buf_t::pending_receive_queue_t receive_queue{};

  mutex_t send_mutex{};
  io_buf_t::pending_send_queue_t send_queue{};
  bool listen_writable = false;


  async_worker_t (socket_t &socket, io_service_t &io_service) noexcept
    : socket(socket)
    , io_service(io_service)
  {}


  ~async_worker_t ()
  {
    while (auto ev = receive_queue.try_pop())
    {
      ev->error = std::make_error_code(std::errc::operation_canceled);
      ev->context->ready(ev);
    }
    while (auto ev = send_queue.try_pop())
    {
      ev->error = std::make_error_code(std::errc::operation_canceled);
      ev->context->ready(ev);
    }
  }


  void push_send (io_buf_t *io_buf) noexcept;
  void send (io_context_t &context, uint16_t flags) noexcept;
  void receive (io_context_t &context, uint16_t flags) noexcept;
};


void delete_async_worker (async_worker_t *async) noexcept
{
  delete async;
}


void async_worker_t::push_send (io_buf_t *io_buf) noexcept
{
  lock_t lock(send_mutex);

  send_queue.push(io_buf);

  if (!listen_writable)
  {
#if __sal_os_darwin

    std::array<struct ::kevent, 1> changes;
    EV_SET(&changes[0],
      socket.handle,
      EVFILT_WRITE,
      EV_ADD,
      0,
      0,
      &socket
    );
    (void)::kevent(io_service.queue,
      changes.data(), changes.size(),
      nullptr, 0,
      nullptr
    );

#elif __sal_os_linux

    struct ::epoll_event change;
    change.events = EPOLLIN | EPOLLOUT | EPOLLET;
    change.data.ptr = &socket;

    (void)::epoll_ctl(io_service.queue,
      EPOLL_CTL_MOD,
      socket.handle,
      &change
    );

#endif

    listen_writable = true;
  }
}


void async_worker_t::send (io_context_t &context, uint16_t flags) noexcept
{
  lock_t lock(send_mutex);

  while (auto *io_buf = send_queue.try_pop())
  {
    if (io_buf->send(socket, flags))
    {
      context.ready(io_buf);
    }
    else
    {
      send_queue.push(io_buf);
      return;
    }
  }

  if (listen_writable)
  {
#if __sal_os_darwin

    std::array<struct ::kevent, 1> changes;
    EV_SET(&changes[0],
      socket.handle,
      EVFILT_WRITE,
      EV_DELETE,
      0,
      0,
      &socket
    );
    (void)::kevent(io_service.queue,
      changes.data(), changes.size(),
      nullptr, 0,
      nullptr
    );

#elif __sal_os_linux

    struct ::epoll_event change;
    change.events = EPOLLIN | EPOLLET;
    change.data.ptr = &socket;

    (void)::epoll_ctl(io_service.queue,
      EPOLL_CTL_MOD,
      socket.handle,
      &change
    );

#endif

    listen_writable = false;
  }
}


void async_worker_t::receive (io_context_t &context, uint16_t flags) noexcept
{
  lock_t lock(receive_mutex);

  while (auto *io_buf = receive_queue.try_pop())
  {
    if (io_buf->receive(socket, flags))
    {
      context.ready(io_buf);
    }
    else
    {
      receive_queue.push(io_buf);
      break;
    }
  }
}


namespace {

#if __sal_os_darwin

using namespace std::chrono;

inline ::timespec *to_timespec (::timespec *ts, const milliseconds &timeout)
  noexcept
{
  if (timeout != (milliseconds::max)())
  {
    auto s = duration_cast<seconds>(timeout);
    ts->tv_nsec = duration_cast<nanoseconds>(timeout - s).count();
    ts->tv_sec = s.count();
    return ts;
  }
  return nullptr;
}

inline int make_poller () noexcept
{
  return ::kqueue();
}

#elif __sal_os_linux

inline int make_poller () noexcept
{
  return ::epoll_create1(0);
}

#endif

} // namespace


io_service_t::io_service_t (std::error_code &error) noexcept
  : queue(make_poller())
{
  if (queue == -1)
  {
    error.assign(errno, std::generic_category());
  }
}


io_service_t::~io_service_t () noexcept
{
  ::close(queue);
}


void io_service_t::associate (socket_t &socket, std::error_code &error)
  noexcept
{
  if (socket.async)
  {
    error = make_error_code(socket_errc_t::already_associated);
    return;
  }

  socket.async.reset(new(std::nothrow) async_worker_t(socket, *this));
  if (!socket.async)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return;
  }

#if __sal_os_darwin

  std::array<struct ::kevent, 1> changes;
  EV_SET(&changes[0],
    socket.handle,
    EVFILT_READ,
    EV_ADD | EV_CLEAR,
    0,
    0,
    &socket
  );

  auto result = ::kevent(queue,
    changes.data(), changes.size(),
    nullptr, 0,
    nullptr
  );

#elif __sal_os_linux

  struct ::epoll_event change;
  change.events = EPOLLIN | EPOLLET;
  change.data.ptr = &socket;

  auto result = ::epoll_ctl(queue,
    EPOLL_CTL_ADD,
    socket.handle,
    &change
  );

#endif

  if (result == -1)
  {
    error.assign(errno, std::generic_category());
    socket.async.reset();
  }
}


bool io_buf_t::receive (socket_t &socket, uint16_t /*flags*/) noexcept
{
  error.clear();

  if (request_id == async_receive_from_t::type_id())
  {
    auto r = static_cast<async_receive_from_t *>(this);
    transferred = socket.receive_from(begin, end - begin,
      &r->address, &r->address_size,
      r->flags,
      error
    );
  }
  else if (request_id == async_receive_t::type_id())
  {
    auto r = static_cast<async_receive_t *>(this);
    transferred = socket.receive(begin, end - begin,
      r->flags,
      error
    );
  }
  else if (request_id == async_accept_t::type_id())
  {
    auto r = static_cast<async_accept_t *>(this);
    size_t remote_address_size = sizeof(*r->remote_address);
    r->accepted = socket.accept(r->remote_address, &remote_address_size,
      false,
      r->error
    );
  }

  return error != std::errc::operation_would_block;
}


bool io_buf_t::send (socket_t &socket, uint16_t flags) noexcept
{
  error.clear();

  if (request_id == async_send_to_t::type_id())
  {
    auto r = static_cast<async_send_to_t *>(this);
    transferred = socket.send_to(begin, end - begin,
      &r->address, r->address_size,
      r->flags,
      error
    );
  }
  else if (request_id == async_send_t::type_id())
  {
    auto r = static_cast<async_send_t *>(this);
    transferred = socket.send(begin, end - begin,
      r->flags,
      error
    );
  }
  else if (request_id == async_connect_t::type_id())
  {
#if __sal_os_darwin
    if (flags & EV_EOF)
    {
      error = std::make_error_code(std::errc::connection_refused);
    }
#elif __sal_os_linux
    if (flags & EPOLLERR)
    {
      int data;
      socklen_t size = sizeof(data);
      auto result = ::getsockopt(socket.handle,
        SOL_SOCKET,
        SO_ERROR,
        &data, &size
      );
      if (result == 0)
      {
        error.assign(data, std::generic_category());
      }
      else
      {
        error.assign(errno, std::generic_category());
      }
    }
#endif

    return true;
  }

  return error != std::errc::operation_would_block;
}


io_buf_t *io_context_t::get (const std::chrono::milliseconds &timeout_ms,
  std::error_code &error) noexcept
{
  error.clear();

  if (auto io_buf = try_get())
  {
    return io_buf;
  }

#if __sal_os_darwin

  ::timespec ts, *timeout = to_timespec(&ts, timeout_ms);
  std::array<struct ::kevent, io_service_t::max_events_per_wait> events;

  do
  {
    auto event_count = ::kevent(io_service.queue,
      nullptr, 0,
      events.data(), max_events_per_wait,
      timeout
    );

    if (event_count == -1)
    {
      error.assign(errno, std::generic_category());
      return nullptr;
    }

    for (auto ev = events.begin(), end = ev + event_count; ev != end;  ++ev)
    {
      auto &socket = *static_cast<socket_t *>(ev->udata);
      if (ev->filter == EVFILT_READ)
      {
        socket.async->receive(*this, ev->flags);
      }
      else if (ev->filter == EVFILT_WRITE)
      {
        socket.async->send(*this, ev->flags);
      }
    }

    if (auto io_buf = try_get())
    {
      return io_buf;
    }
  } while (!timeout);

#elif __sal_os_linux

  int timeout = timeout_ms == timeout_ms.max() ? -1 : timeout_ms.count();
  std::array<::epoll_event, io_service_t::max_events_per_wait> events;

  do
  {
    auto event_count = ::epoll_wait(io_service.queue,
      events.data(), max_events_per_wait,
      timeout
    );

    if (event_count == -1)
    {
      error.assign(errno, std::generic_category());
      return nullptr;
    }

    for (auto ev = events.begin(), end = ev + event_count;  ev != end;  ++ev)
    {
      auto &socket = *static_cast<socket_t *>(ev->data.ptr);
      if (ev->events & EPOLLIN)
      {
        socket.async->receive(*this, ev->events);
      }
      if (ev->events & EPOLLOUT)
      {
        socket.async->send(*this, ev->events);
      }
    }

    if (auto io_buf = try_get())
    {
      return io_buf;
    }
  } while (timeout == -1);

#endif

  return nullptr;
}


void async_receive_from_t::start (socket_t &socket, message_flags_t flags)
  noexcept
{
  flags |= MSG_DONTWAIT;

  error.clear();
  address_size = sizeof(address);
  transferred = socket.receive_from(begin, end - begin,
    &address, &address_size,
    flags,
    error
  );
  if (error != std::errc::operation_would_block)
  {
    context->ready(this);
    return;
  }

  this->flags = flags;
  socket.async->receive_queue.push(this);
}


void async_receive_t::start (socket_t &socket, message_flags_t flags) noexcept
{
  flags |= MSG_DONTWAIT;

  error.clear();
  transferred = socket.receive(begin, end - begin,
    flags,
    error
  );
  if (error != std::errc::operation_would_block)
  {
    context->ready(this);
    return;
  }

  this->flags = flags;
  socket.async->receive_queue.push(this);
}


void async_send_to_t::start (socket_t &socket,
  const void *address, size_t address_size,
  message_flags_t flags) noexcept
{
  flags |= MSG_DONTWAIT;

  error.clear();
  transferred = socket.send_to(begin, end - begin,
    address, address_size,
    flags,
    error
  );
  if (error != std::errc::operation_would_block)
  {
    context->ready(this);
    return;
  }

  std::memcpy(&this->address, address, address_size);
  this->address_size = address_size;
  this->flags = flags;
  socket.async->push_send(this);
}


void async_send_t::start (socket_t &socket, message_flags_t flags) noexcept
{
  flags |= MSG_DONTWAIT;

  error.clear();
  transferred = socket.send(begin, end - begin, flags, error);
  if (error != std::errc::operation_would_block)
  {
    context->ready(this);
    return;
  }

  this->flags = flags;
  socket.async->push_send(this);
}


void async_connect_t::start (socket_t &socket,
  const void *address, size_t address_size) noexcept
{
  error.clear();
  socket.connect(address, address_size, error);
  if (error != std::errc::operation_in_progress)
  {
    context->ready(this);
    return;
  }

  socket.async->push_send(this);
}


void async_accept_t::start (socket_t &socket, int family) noexcept
{
  (void)family;

  auto *addresses = reinterpret_cast<sockaddr_storage *>(begin);
  remote_address = &addresses[0];
  local_address = &addresses[1];

  auto remote_address_size = sizeof(*remote_address);

  error.clear();
  accepted = socket.accept(remote_address, &remote_address_size,
    false,
    error
  );
  if (error != std::errc::operation_would_block)
  {
    context->ready(this);
    return;
  }

  socket.async->receive_queue.push(this);
}


void async_accept_t::finish (std::error_code &error) noexcept
{
  error = this->error;
  if (!error)
  {
    socklen_t local_address_size = sizeof(*local_address);
    auto result = ::getsockname(accepted,
      reinterpret_cast<sockaddr *>(local_address),
      &local_address_size
    );
    if (result == -1)
    {
      error.assign(errno, std::generic_category());
    }
  }
}


#endif


}} // namespace net::__bits


__sal_end
