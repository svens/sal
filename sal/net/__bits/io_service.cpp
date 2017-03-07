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
  #include <sys/time.h>
  #include <unistd.h>
#endif


#include <iostream>


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
    error.clear();
    context->immediate_completions.push(this);
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
  else
  {
    error.assign(e, std::system_category());
  }
  context->immediate_completions.push(this);
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
    context->immediate_completions.push(this);
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
  context->immediate_completions.push(this);
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
    context->immediate_completions.push(this);
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
    context->immediate_completions.push(this);
    return;
  }

  auto e = ::WSAGetLastError();
  if (e == ERROR_IO_PENDING)
  {
    return;
  }

  error.assign(e, std::system_category());
  context->immediate_completions.push(this);
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
  auto result = ::CreateIoCompletionPort(
    reinterpret_cast<HANDLE>(socket.native_handle),
    iocp,
    0,
    0
  );
  if (!result)
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
    if (!NT_SUCCESS(status))
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
    }

    io_buf->transferred = entry.dwNumberOfBytesTransferred;
    io_buf->context = this;
    return io_buf;
  }
  return immediate_completions.try_pop();
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


struct async_worker_t
{
  io_buf_t::queue_t pending_receive{}, pending_send{};

  ~async_worker_t ()
  {
    while (auto ev = pending_receive.try_pop())
    {
      ev->error = std::make_error_code(std::errc::operation_canceled);
      ev->context->immediate_completions.push(ev);
    }

    // TODO: send?
  }
};


void delete_async_worker (async_worker_t *async_worker) noexcept
{
  delete async_worker;
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
  if (queue != -1)
  {
    ::close(queue);
  }
}


void io_service_t::associate (socket_t &socket, std::error_code &error)
  noexcept
{
  sal_assert(socket.async_worker == nullptr);

  socket.async_worker.reset(new(std::nothrow) async_worker_t);
  if (!socket.async_worker)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return;
  }

  struct ::kevent change;
  EV_SET(&change,
    socket.native_handle,
    EVFILT_READ | EVFILT_WRITE,
    EV_ADD | EV_CLEAR,
    0,
    0,
    &socket
  );

  if (::kevent(queue, &change, 1, nullptr, 0, nullptr) == -1)
  {
    error.assign(errno, std::generic_category());
    socket.async_worker.reset();
    return;
  }

  socket.non_blocking(true, error);
}


io_buf_t *io_context_t::receive () noexcept
{
  if (event->data)
  {
    auto &socket = *static_cast<socket_t *>(event->udata);
    auto *r = static_cast<async_receive_from_t *>(
      socket.async_worker->pending_receive.try_pop()
    );
    if (r)
    {
      r->error.clear();
      r->transferred = socket.receive_from(
        r->begin, r->end - r->begin,
        &r->address, &r->address_size,
        r->flags,
        r->error
      );
      if (r->error != std::errc::operation_would_block)
      {
        event->data -= r->transferred;
        return r;
      }
      socket.async_worker->pending_receive.push(r);
    }
  }
  return nullptr;
}


io_buf_t *io_context_t::send () noexcept
{
  return nullptr;
}


io_buf_t *io_context_t::try_get () noexcept
{
  while (event != last_event)
  {
    if (auto *io_buf = event->filter == EVFILT_READ ? receive() : send())
    {
      return io_buf;
    }
    ++event;
  }
  return immediate_completions.try_pop();
}


io_buf_t *io_context_t::get (const std::chrono::milliseconds &timeout,
  std::error_code &error) noexcept
{
  if (auto io_buf = try_get())
  {
    return io_buf;
  }

  using namespace std::chrono;

  struct ::timespec ts, *t = nullptr;
  if (timeout != milliseconds::max())
  {
    auto s = duration_cast<seconds>(timeout);
    ts.tv_nsec = duration_cast<nanoseconds>(timeout - s).count();
    ts.tv_sec = s.count();
    t = &ts;
  }

  event = events.begin();
  auto n = ::kevent(io_service.queue,
    nullptr, 0,
    events.data(), max_events_per_wait,
    t
  );
  if (n != -1)
  {
    last_event = event + n;
    return try_get();
  }

  error.assign(errno, std::generic_category());
  last_event = event;
  return nullptr;
}


void async_receive_from_t::start (socket_t &socket, message_flags_t flags)
  noexcept
{
  error.clear();

  address_size = sizeof(address);
  transferred = socket.receive_from(begin, end - begin,
    &address, &address_size,
    flags,
    error
  );

  if (error != std::errc::operation_would_block)
  {
    context->immediate_completions.push(this);
  }
  else
  {
    this->flags = flags;
    socket.async_worker->pending_receive.push(this);
  }
}


void async_send_to_t::start (socket_t &socket,
  const void *address, size_t address_size,
  message_flags_t flags) noexcept
{
  (void)socket;
  (void)address;
  (void)address_size;
  (void)flags;
}


#endif


}} // namespace net::__bits


__sal_end
