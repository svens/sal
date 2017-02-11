#include <sal/net/__bits/io_service.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/error.hpp>
#include <mutex>

#if __sal_os_windows
  #include <mswsock.h>
#endif


__sal_begin


#if __sal_os_windows


namespace net { namespace __bits {


namespace {


LPFN_CONNECTEX ConnectEx = nullptr;
LPFN_ACCEPTEX AcceptEx = nullptr;
LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockaddrs = nullptr;


template <typename Function>
void load_fn (Function *fn, GUID id, SOCKET socket, std::error_code &result)
  noexcept
{
  if (result)
  {
    return;
  }

  DWORD bytes;
  auto i = ::WSAIoctl(socket,
    SIO_GET_EXTENSION_FUNCTION_POINTER,
    &id, sizeof(id),
    fn, sizeof(fn),
    &bytes,
    nullptr,
    nullptr
  );
  if (i == SOCKET_ERROR)
  {
    result.assign(::WSAGetLastError(), std::system_category());
  }
}


void internal_setup (std::error_code &result) noexcept
{
  if (!result)
  {
    result = sal::net::init();
  }

  if (!result)
  {
    auto tmp = ::socket(AF_INET, SOCK_STREAM, 0);
    load_fn(&ConnectEx, WSAID_CONNECTEX, tmp, result);
    load_fn(&AcceptEx, WSAID_ACCEPTEX, tmp, result);
    load_fn(&GetAcceptExSockaddrs, WSAID_GETACCEPTEXSOCKADDRS, tmp, result);
    ::closesocket(tmp);
  }
}


} // namespace


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


bool io_buf_t::io_result (int result) noexcept
{
  if (result == 0)
  {
    // completed immediately, caller still owns data
    error.clear();
    return true;
  }

  auto e = ::WSAGetLastError();
  if (e == WSA_IO_PENDING)
  {
    // pending, OS owns data
    return false;
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
  return true;
}


bool async_receive_t::start (socket_t &socket, message_flags_t flags) noexcept
{
  DWORD flags_ = flags;
  auto buf = to_buf();
  return io_result(
    ::WSARecv(socket.native_handle,
      &buf, 1,
      &transferred,
      &flags_,
      this,
      nullptr
    )
  );
}


bool async_receive_from_t::start (socket_t &socket, message_flags_t flags)
  noexcept
{
  address_size = sizeof(address);

  DWORD flags_ = flags;
  auto buf = to_buf();
  return io_result(
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


bool async_send_to_t::start (socket_t &socket,
  const void *address, size_t address_size,
  message_flags_t flags) noexcept
{
  auto buf = to_buf();
  return io_result(
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


bool async_send_t::start (socket_t &socket, message_flags_t flags) noexcept
{
  auto buf = to_buf();
  return io_result(
    ::WSASend(socket.native_handle,
      &buf, 1,
      &transferred,
      flags,
      this,
      nullptr
    )
  );
}


bool async_connect_t::start (socket_t &socket,
  const void *address, size_t address_size) noexcept
{
  native_handle = socket.native_handle;
  auto result = (*ConnectEx)(native_handle,
    static_cast<const sockaddr *>(address),
    static_cast<int>(address_size),
    nullptr, 0, nullptr,
    this
  );

  if (result == TRUE)
  {
    // immediate completion, caller still owns op
    error.clear();
    return true;
  }

  auto e = ::WSAGetLastError();
  if (e == WSA_IO_PENDING)
  {
    // pending, OS now owns op
    return false;
  }

  // failed immediately, caller owns op
  error.assign(e, std::system_category());
  return true;
}


void async_connect_t::finish (std::error_code &result) noexcept
{
  switch (error.value())
  {
    case 0:
      ::setsockopt(native_handle,
        SOL_SOCKET,
        SO_UPDATE_CONNECT_CONTEXT,
        nullptr,
        0
      );
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


}} // namespace net::__bits


#endif // __sal_os_windows


__sal_end
