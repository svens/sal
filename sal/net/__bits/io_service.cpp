#if defined(_WIN32) || defined(_WIN64)
  #define WIN32_NO_STATUS
  #include <windows.h>
  #undef WIN32_NO_STATUS
  #include <winternl.h>
  #include <ntstatus.h>
  #pragma comment(lib, "ntdll")
#endif

#include <sal/net/__bits/io_service.hpp>
#include <sal/net/error.hpp>
#include <sal/net/fwd.hpp>
#include <mutex>

#if __sal_os_windows
  #include <mswsock.h>
#elif __sal_os_darwin
  #include <sal/assert.hpp>
  #include <sal/spinlock.hpp>
  #include <sys/time.h>
  #include <unistd.h>
#endif

#include <sal/thread.hpp>
#include <iostream>


#define NONE "\033[0m"
#define BRIGHT "\033[1m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"


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
      error = make_error_code(socket_errc_t::orderly_shutdown);
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
    error = make_error_code(socket_errc_t::orderly_shutdown);
  }
  else if (e == WSAENOTSOCK)
  {
    error.assign(WSAEBADF, std::system_category());
  }
  else if (e == WSAENOTCONN)
  {
    error.assign(WSAEDESTADDRREQ, std::system_category());
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
    ::WSARecv(socket.native_handle,
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
    ::WSARecvFrom(socket.native_handle,
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
    ::WSASendTo(socket.native_handle,
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
    ::WSASend(socket.native_handle,
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
  handle = socket.native_handle;
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
  acceptor = socket.native_handle;
  accepted = new_socket.native_handle;
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
    reinterpret_cast<HANDLE>(socket.native_handle),
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
        io_buf->error = make_error_code(socket_errc_t::orderly_shutdown);
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


#elif __sal_os_darwin


namespace {

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

inline void printf (...) noexcept
{
}

} // namespace


struct async_worker_t
{
  using mutex_t = spinlock_t;
  using lock_t = std::lock_guard<mutex_t>;

  mutex_t receive_mutex{};
  io_buf_t::pending_receive_queue_t receive_queue{};

  mutex_t send_mutex{};
  io_buf_t::pending_send_queue_t send_queue{};
  bool listen_writable = false;


  ~async_worker_t ()
  {
    size_t pending_sends = 0, pending_receives = 0;
    while (auto ev = receive_queue.try_pop())
    {
      ++pending_receives;
      ev->error = std::make_error_code(std::errc::operation_canceled);
      ev->context->ready(ev);
    }
    while (auto ev = send_queue.try_pop())
    {
      ++pending_sends;
      ev->error = std::make_error_code(std::errc::operation_canceled);
      ev->context->ready(ev);
    }
    printf("\n*\n* receives=%zu\n* sends=%zu\n*\n\n", pending_receives, pending_sends);
  }


  void push_receive (io_buf_t *io_buf) noexcept
  {
    receive_queue.push(io_buf);
  }


  io_buf_t *pop_receive () noexcept
  {
    lock_t lock(receive_mutex);
    return receive_queue.try_pop();
  }


  void push_send (io_buf_t *io_buf) noexcept
  {
    send_queue.push(io_buf);
  }


  io_buf_t *pop_send () noexcept
  {
    lock_t lock(send_mutex);
    return send_queue.try_pop();
  }
};


void delete_async_worker (async_worker_t *async) noexcept
{
  delete async;
}


io_service_t::io_service_t (std::error_code &error) noexcept
  : queue(::kqueue())
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

  socket.async.reset(new(std::nothrow) async_worker_t);
  if (!socket.async)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return;
  }

  std::array<struct ::kevent, 2> changes;
  EV_SET(&changes[0],
    socket.native_handle,
    EVFILT_READ,
    EV_ADD | EV_CLEAR,
    0,
    0,
    &socket
  );
  EV_SET(&changes[1],
    socket.native_handle,
    EVFILT_WRITE,
    EV_ADD | EV_CLEAR,
    0,
    0,
    &socket
  );

  if (::kevent(queue, changes.data(), changes.size(), nullptr, 0, nullptr) == -1)
  {
    error.assign(errno, std::generic_category());
    socket.async.reset();
  }
}


bool io_buf_t::retry_receive (socket_t &socket) noexcept
{
  error.clear();

  const char *f = "<unknown>";
  if (request_id == async_receive_from_t::type_id())
  {
    f = "receive_from";
    auto r = static_cast<async_receive_from_t *>(this);
    transferred = socket.receive_from(begin, end - begin,
      &r->address, &r->address_size,
      r->flags,
      error
    );
  }
  else if (request_id == async_receive_t::type_id())
  {
    f = "receive";
    auto r = static_cast<async_receive_t *>(this);
    transferred = socket.receive(begin, end - begin,
      r->flags,
      error
    );
  }

  if (error && error != std::errc::operation_would_block)
  {
    printf(BRIGHT RED "%u\t%s=(%d, %p, %zu)\n" NONE, sal::this_thread::get_id(), f, error.value(), (void *)begin, (end - begin));
  }

  return error != std::errc::operation_would_block;
}


bool io_buf_t::retry_send (socket_t &socket, uint16_t flags) noexcept
{
  error.clear();

  const char *f = "<unknown>";
  if (request_id == async_send_to_t::type_id())
  {
    f = "send_to";
    auto r = static_cast<async_send_to_t *>(this);
    transferred = socket.send_to(begin, end - begin,
      &r->address, r->address_size,
      r->flags,
      error
    );
  }
  else if (request_id == async_send_t::type_id())
  {
    f = "send";
    auto r = static_cast<async_send_t *>(this);
    transferred = socket.send(begin, end - begin,
      r->flags,
      error
    );
  }
  else if (request_id == async_connect_t::type_id())
  {
    if (flags & EV_EOF)
    {
      error = std::make_error_code(std::errc::connection_refused);
      printf(BRIGHT RED "%u\tConnection refused\n" NONE, sal::this_thread::get_id());
    }
    else
    {
      printf(BRIGHT GREEN "%u\tConnected\n" NONE, sal::this_thread::get_id());
    }
    return true;
  }

  if (error && error != std::errc::operation_would_block)
  {
    printf(BRIGHT RED "%u\t%s=(%d, %p, %zu)\n" NONE, sal::this_thread::get_id(), f, error.value(), (void *)begin, (end - begin));
  }

  return error != std::errc::operation_would_block;
}


io_buf_t *io_context_t::retry_receive (socket_t &socket) noexcept
{
  if (auto *io_buf = socket.async->pop_receive())
  {
    if (io_buf->retry_receive(socket))
    {
      printf(GREEN "%u\tReceived=%zu" NONE "\n", sal::this_thread::get_id(), io_buf->transferred);
      return io_buf;
    }
    printf(RED "%u\tReceive retry" NONE "\n", sal::this_thread::get_id());
    socket.async->push_receive(io_buf);
  }
  else
  {
    printf(MAGENTA "%u\tReceive empty" NONE "\n", sal::this_thread::get_id());
  }

  return nullptr;
}


io_buf_t *io_context_t::retry_send (socket_t &socket) noexcept
{
  if (auto *io_buf = socket.async->pop_send())
  {
    if (io_buf->retry_send(socket, event->flags))
    {
      printf(GREEN "%u\tSent=%zu" NONE "\n", sal::this_thread::get_id(), io_buf->transferred);
      return io_buf;
    }
    printf(RED "%u\tSend retry" NONE "\n", sal::this_thread::get_id());
    socket.async->push_send(io_buf);
  }
  else
  {
    printf(MAGENTA "%u\tSend empty" NONE "\n", sal::this_thread::get_id());
  }

  return nullptr;
}


io_buf_t *io_context_t::try_get () noexcept
{
  while (event != last_event)
  {
    auto &socket = *static_cast<socket_t *>(event->udata);

    io_buf_t *io_buf = nullptr;
    if (event->filter == EVFILT_READ)
    {
      io_buf = retry_receive(socket);
    }
    else if (event->filter == EVFILT_WRITE)
    {
      io_buf = retry_send(socket);
    }
    else
    {
      printf(BRIGHT RED "%u\tFilter=%hd, Flags=%hu, Data=%ld" NONE "\n", sal::this_thread::get_id(), event->filter, event->flags, event->data);
    }

    if (io_buf)
    {
      return io_buf;
    }

    ++event;
  }
  return completed.try_pop();
}


io_buf_t *io_context_t::get (const std::chrono::milliseconds &timeout_ms,
  std::error_code &error) noexcept
{
  if (auto io_buf = try_get())
  {
    return io_buf;
  }

  printf("%u\tWait\n", sal::this_thread::get_id());

  for (::timespec ts, *timeout = to_timespec(&ts, timeout_ms);  /**/;  /**/)
  {
    auto event_count = ::kevent(io_service.queue,
      nullptr, 0,
      events.data(), max_events_per_wait,
      timeout
    );

    if (event_count > -1)
    {
      event = events.begin();
      last_event = event + event_count;

      if (auto io_buf = try_get())
      {
        return io_buf;
      }
      else if (!timeout)
      {
        continue;
      }
    }
    else
    {
      error.assign(errno, std::generic_category());
      printf(BRIGHT RED "%u\tWait=%d" NONE "\n", sal::this_thread::get_id(), error.value());
    }

    return nullptr;
  }
}


void async_receive_from_t::start (socket_t &socket, message_flags_t flags)
  noexcept
{
  error.clear();

  flags |= MSG_DONTWAIT;
  address_size = sizeof(address);
  transferred = socket.receive_from(begin, end - begin,
    &address, &address_size,
    flags,
    error
  );

  if (error != std::errc::operation_would_block)
  {
    context->ready(this);
  }
  else
  {
    this->flags = flags;
    socket.async->push_receive(this);
  }
}


void async_receive_t::start (socket_t &socket, message_flags_t flags) noexcept
{
  error.clear();

  flags |= MSG_DONTWAIT;
  transferred = socket.receive(begin, end - begin,
    flags,
    error
  );

  if (error != std::errc::operation_would_block)
  {
    context->ready(this);
  }
  else
  {
    this->flags = flags;
    socket.async->push_receive(this);
  }
}


void async_send_to_t::start (socket_t &socket,
  const void *address, size_t address_size,
  message_flags_t flags) noexcept
{
  error.clear();

  flags |= MSG_DONTWAIT;
  transferred = socket.send_to(begin, end - begin,
    address, address_size,
    flags,
    error
  );

  if (error != std::errc::operation_would_block)
  {
    printf("%u\tSent\n", sal::this_thread::get_id());
    context->ready(this);
  }
  else
  {
    printf(YELLOW "%u\tPending" NONE "\n", sal::this_thread::get_id());
    std::memcpy(&this->address, address, address_size);
    this->address_size = address_size;
    this->flags = flags;
    socket.async->push_send(this);
  }
}


void async_send_t::start (socket_t &socket, message_flags_t flags) noexcept
{
  error.clear();

  flags |= MSG_DONTWAIT;
  transferred = socket.send(begin, end - begin, flags, error);

  if (error != std::errc::operation_would_block)
  {
    if (error == std::errc::broken_pipe)
    {
      error = make_error_code(socket_errc_t::orderly_shutdown);
    }
    context->ready(this);
  }
  else
  {
    this->flags = flags;
    socket.async->push_send(this);
  }
}


void async_connect_t::start (socket_t &socket,
  const void *address, size_t address_size) noexcept
{
  error.clear();

  socket.connect(address, address_size, error);
  if (error != std::errc::operation_in_progress)
  {
    context->ready(this);
  }
  else
  {
    socket.async->push_send(this);
  }
}


#endif


}} // namespace net::__bits


__sal_end
