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
  if (completion_index < completion_count)
  {
    auto &entry = completions[completion_index++];
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

  completion_index = 0;
  auto succeeded = ::GetQueuedCompletionStatusEx(io_service.iocp,
    completions.data(), max_completion_count, &completion_count,
    static_cast<DWORD>(timeout.count()),
    false
  );
  if (succeeded)
  {
    return try_get();
  }

  auto e = ::GetLastError();
  if (e != WAIT_TIMEOUT)
  {
    error.assign(e, std::system_category());
  }

  completion_count = 0;
  return nullptr;
}


#elif __sal_os_darwin


void io_service_t::associate (socket_t &, std::error_code &) noexcept
{
}


io_buf_t *io_context_t::try_get () noexcept
{
  return nullptr;
}


io_buf_t *io_context_t::get (const std::chrono::milliseconds &,
  std::error_code &) noexcept
{
  return nullptr;
}


#endif


}} // namespace net::__bits


__sal_end
