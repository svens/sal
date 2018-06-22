#include <sal/__bits/platform_sdk.hpp>
#include <sal/net/__bits/socket.hpp>

#if __sal_os_macos || __sal_os_linux
  #include <fcntl.h>
  #include <poll.h>
  #include <sys/ioctl.h>
  #include <unistd.h>
#elif __sal_os_windows
  #include <mutex>
  #include <mswsock.h>
#endif


__sal_begin


namespace net { namespace __bits {


#if __sal_os_windows // {{{1


namespace {


struct winsock_t
  : public RIO_EXTENSION_FUNCTION_TABLE
{
  winsock_t () noexcept
    : RIO_EXTENSION_FUNCTION_TABLE{sizeof(winsock_t)}
  {
    init_lib();
  }

  ~winsock_t () noexcept
  {
    (void)::WSACleanup();
  }
} winsock{};


template <typename T>
inline T check_call (T result, std::error_code &error) noexcept
{
  if (result != SOCKET_ERROR)
  {
    error.clear();
  }
  else
  {
    auto e = ::WSAGetLastError();
    if (e == WSAENOTSOCK)
    {
      e = WSAEBADF;
    }
    error.assign(e, std::system_category());
  }
  return result;
}

#define call(func,error,...) check_call(func(__VA_ARGS__), error)


void init_winsock (std::error_code &init_result) noexcept
{
  WSADATA wsa;
  init_result.assign(
    ::WSAStartup(MAKEWORD(2, 2), &wsa),
    std::system_category()
  );

  if (!init_result)
  {
    GUID rio_guid = WSAID_MULTIPLE_RIO;
    DWORD bytes;

    auto s = ::socket(AF_INET, SOCK_STREAM, 0);
    call(::WSAIoctl, init_result, s,
      SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER,
      &rio_guid, sizeof(rio_guid),
      &winsock, sizeof(winsock),
      &bytes,
      nullptr,
      nullptr
    );
    (void)::closesocket(s);
  }
}


} // namespace


const std::error_code &init_lib () noexcept
{
  static std::once_flag once;
  static std::error_code init_result;
  std::call_once(once, &init_winsock, std::ref(init_result));
  return init_result;
}


void socket_t::open (int domain, int type, int protocol,
  std::error_code &error) noexcept
{
  handle = call(::WSASocketW,
    error,
    domain,
    type,
    protocol,
    nullptr,
    0,
    0
  );
}


void socket_t::close (std::error_code &error) noexcept
{
  call(::closesocket, error, handle);
  handle = invalid;
}


void socket_t::bind (const void *address, size_t address_size,
  std::error_code &error) noexcept
{
  call(::bind,
    error,
    handle,
    static_cast<const sockaddr *>(address),
    static_cast<socklen_t>(address_size)
  );
}


void socket_t::listen (int backlog, std::error_code &error) noexcept
{
  call(::listen, error, handle, backlog);
}


socket_t::handle_t socket_t::accept (void *address, size_t *address_size,
  bool enable_connection_aborted, std::error_code &error) noexcept
{
  socklen_t size{}, *size_p = nullptr;
  if (address_size)
  {
    size = static_cast<socklen_t>(*address_size);
    size_p = &size;
  }

retry:
  auto new_socket = call(::accept,
    error,
    handle,
    static_cast<sockaddr *>(address),
    size_p
  );

  if (new_socket != invalid)
  {
    if (address_size)
    {
      *address_size = size;
    }
    return new_socket;
  }

  if (error == std::errc::connection_aborted && !enable_connection_aborted)
  {
    goto retry;
  }

  return 0;
}


void socket_t::connect (const void *address, size_t address_size,
  std::error_code &error) noexcept
{
  call(::connect,
    error,
    handle,
    static_cast<const sockaddr *>(address),
    static_cast<socklen_t>(address_size)
  );
}


bool socket_t::wait (wait_t what, int timeout_ms, std::error_code &error)
  noexcept
{
  pollfd fd{};
  fd.fd = handle;
  fd.events = what == wait_t::read ? POLLIN : POLLOUT;

  auto event_count = ::WSAPoll(&fd, 1, timeout_ms);
  if (event_count == 1)
  {
    return (fd.revents & fd.events) != 0;
  }
  else if (event_count == -1)
  {
    check_call(invalid, error);
  }

  return false;
}


size_t socket_t::receive (void *data, size_t data_size, message_flags_t flags,
  std::error_code &error) noexcept
{
  WSABUF buf;
  buf.buf = static_cast<char *>(data);
  buf.len = static_cast<ULONG>(data_size);

  DWORD transferred{};
  DWORD recv_flags = flags;

  auto result = call(::WSARecv,
    error,
    handle,
    &buf, 1,
    &transferred,
    &recv_flags,
    nullptr,
    nullptr
  );

  if (result != SOCKET_ERROR || error.value() == WSAESHUTDOWN)
  {
    if (transferred)
    {
      return transferred;
    }
    error = make_error_code(std::errc::broken_pipe);
  }

  return 0;
}


size_t socket_t::receive_from (void *data, size_t data_size,
  void *address, size_t *address_size,
  message_flags_t flags,
  std::error_code &error) noexcept
{
  WSABUF buf;
  buf.buf = static_cast<char *>(data);
  buf.len = static_cast<ULONG>(data_size);

  auto tmp_address_size = static_cast<INT>(*address_size);

  DWORD transferred{};
  DWORD recv_flags = flags;

  auto result = call(::WSARecvFrom,
    error,
    handle,
    &buf, 1,
    &transferred,
    &recv_flags,
    static_cast<sockaddr *>(address),
    (address ? &tmp_address_size : nullptr),
    nullptr,
    nullptr
  );

  if (result != SOCKET_ERROR)
  {
    *address_size = tmp_address_size;
    return transferred;
  }

  return 0;
}


size_t socket_t::send (const void *data, size_t data_size, message_flags_t flags,
  std::error_code &error) noexcept
{
  WSABUF buf;
  buf.buf = static_cast<char *>(const_cast<void *>(data));
  buf.len = static_cast<ULONG>(data_size);

  DWORD transferred{};

  auto result = call(::WSASend,
    error,
    handle,
    &buf, 1,
    &transferred,
    flags,
    nullptr,
    nullptr
  );

  if (result != SOCKET_ERROR)
  {
    return transferred;
  }
  else if (error.value() == WSAESHUTDOWN)
  {
    error = make_error_code(std::errc::broken_pipe);
  }

  return 0;
}


size_t socket_t::send_to (const void *data, size_t data_size,
  const void *address, size_t address_size,
  message_flags_t flags,
  std::error_code &error) noexcept
{
  WSABUF buf;
  buf.buf = static_cast<char *>(const_cast<void *>(data));
  buf.len = static_cast<ULONG>(data_size);

  DWORD transferred{};

  auto result = call(::WSASendTo,
    error,
    handle,
    &buf, 1,
    &transferred,
    flags,
    static_cast<const sockaddr *>(address),
    static_cast<int>(address_size),
    nullptr,
    nullptr
  );

  if (result != SOCKET_ERROR)
  {
    return transferred;
  }

  return 0;
}


void socket_t::shutdown (shutdown_t what, std::error_code &error) noexcept
{
  call(::shutdown, error, handle, static_cast<int>(what));
}


void socket_t::remote_endpoint (void *address, size_t *address_size,
  std::error_code &error) const noexcept
{
  auto size = static_cast<socklen_t>(*address_size);
  auto result = call(::getpeername,
    error,
    handle,
    static_cast<sockaddr *>(address), &size
  );
  if (result != -1)
  {
    *address_size = size;
  }
}


void socket_t::local_endpoint (void *address, size_t *address_size,
  std::error_code &error) const noexcept
{
  auto size = static_cast<socklen_t>(*address_size);
  auto result = call(::getsockname,
    error,
    handle,
    static_cast<sockaddr *>(address), &size
  );
  if (result != -1)
  {
    *address_size = size;
  }
}


void socket_t::get_opt (int level, int name, void *data, size_t *size,
  std::error_code &error) const noexcept
{
  auto data_size = static_cast<socklen_t>(*size);
  auto result = call(::getsockopt,
    error,
    handle,
    level, name,
    static_cast<char *>(data), &data_size
  );
  if (result != -1)
  {
    *size = data_size;
  }
}


void socket_t::set_opt (int level, int name, const void *data, size_t size,
  std::error_code &error) noexcept
{
  call(::setsockopt,
    error,
    handle,
    level, name,
    static_cast<const char *>(data), static_cast<socklen_t>(size)
  );
}


bool socket_t::non_blocking (std::error_code &error) const noexcept
{
  error.assign(WSAEOPNOTSUPP, std::system_category());
  return true;
}


void socket_t::non_blocking (bool mode, std::error_code &error) noexcept
{
  unsigned long arg = mode ? 1 : 0;
  call(::ioctlsocket, error, handle, FIONBIO, &arg);
}


size_t socket_t::available (std::error_code &error) const noexcept
{
  unsigned long value{};
  if (call(::ioctlsocket, error, handle, FIONBIO, &value) == SOCKET_ERROR)
  {
    value = 0;
  }
  return value;
}


#elif __sal_os_macos || __sal_os_linux // {{{1


namespace {


template <typename T>
inline T check_call (T result, std::error_code &error) noexcept
{
  if (result != -1)
  {
    error.clear();
  }
  else
  {
    if (errno == EDESTADDRREQ)
    {
      errno = ENOTCONN;
    }
    error.assign(errno, std::generic_category());
  }
  return result;
}

#define call(func,error,...) check_call(func(__VA_ARGS__), error)


} // namspace


const std::error_code &init_lib () noexcept
{
  static const std::error_code no_error{};
  return no_error;
}


void socket_t::open (int domain, int type, int protocol,
  std::error_code &error) noexcept
{
  handle = call(::socket, error, domain, type, protocol);

#if __sal_os_macos
  if (handle != invalid)
  {
    int optval = 1;
    (void)::setsockopt(handle, SOL_SOCKET, SO_NOSIGPIPE,
      &optval, sizeof(optval)
    );
  }
#endif
}


void socket_t::close (std::error_code &error) noexcept
{
  for (;;)
  {
    if (call(::close, error, handle) == 0 || errno != EINTR)
    {
      handle = invalid;
      return;
    }
  }
}


void socket_t::bind (const void *address, size_t address_size,
  std::error_code &error) noexcept
{
  call(::bind,
    error,
    handle,
    static_cast<const sockaddr *>(address),
    static_cast<socklen_t>(address_size)
  );
}


void socket_t::listen (int backlog, std::error_code &error) noexcept
{
  call(::listen, error, handle, backlog);
}


socket_t::handle_t socket_t::accept (void *address, size_t *address_size,
  bool enable_connection_aborted, std::error_code &error) noexcept
{
  socklen_t size{}, *size_p = nullptr;
  if (address_size)
  {
    size = static_cast<socklen_t>(*address_size);
    size_p = &size;
  }

retry:
  auto new_socket = call(::accept,
    error,
    handle,
    static_cast<sockaddr *>(address),
    size_p
  );

  if (new_socket != invalid)
  {
    if (address_size)
    {
#if __sal_os_macos
      // LCOV_EXCL_START
      // kernel bug: instead of ECONNABORTED, we might get size=0
      if (!size)
      {
        if (enable_connection_aborted)
        {
          error.assign(ECONNABORTED, std::generic_category());
          return invalid;
        }
        size = static_cast<socklen_t>(*address_size);
        goto retry;
      }
      // LCOV_EXCL_STOP
#endif
      *address_size = size;
    }

#if __sal_os_macos
    int optval = 1;
    ::setsockopt(new_socket, SOL_SOCKET, SO_NOSIGPIPE,
      &optval, sizeof(optval)
    );
#endif

    return new_socket;
  }

  // LCOV_EXCL_START
  // OS-dependent section

#if __sal_os_linux
  // see accept(2), these are already pending errors
  if (errno == ENETDOWN
    || errno == EPROTO
    || errno == ENOPROTOOPT
    || errno == EHOSTDOWN
    || errno == ENONET
    || errno == EHOSTUNREACH
    || errno == EOPNOTSUPP
    || errno == ENETUNREACH)
  {
    goto retry;
  }
#endif

  if (error == std::errc::connection_aborted && !enable_connection_aborted)
  {
    goto retry;
  }

  // LCOV_EXCL_STOP

  return invalid;
}


void socket_t::connect (const void *address, size_t address_size,
  std::error_code &error) noexcept
{
  call(::connect,
    error,
    handle,
    static_cast<const sockaddr *>(address),
    static_cast<socklen_t>(address_size)
  );
}


bool socket_t::wait (wait_t what, int timeout_ms, std::error_code &error)
  noexcept
{
  if (handle == invalid)
  {
    error.assign(EBADF, std::generic_category());
    return false;
  }

  pollfd fd{};
  fd.fd = handle;
  fd.events = what == wait_t::read ? POLLIN : POLLOUT;

  auto event_count = ::poll(&fd, 1, timeout_ms);
  if (event_count == 1)
  {
#if __sal_os_linux
    // Linux does the "right thing", setting POLLHUP on non-connected sockets
    // unfortunately Darwin & Windows disagree and no way to detect such
    // situation, so simply align after their behaviour
    if (fd.revents & POLLHUP)
    {
      return false;
    }
#endif
    return (fd.revents & fd.events) != 0;
  }

  // LCOV_EXCL_START
  // can't reach this case by feeding incorrect parameters
  else if (event_count == -1)
  {
    check_call(invalid, error);
  }
  // LCOV_EXCL_STOP

  return false;
}


size_t socket_t::receive (void *data, size_t data_size, message_flags_t flags,
  std::error_code &error) noexcept
{
  iovec iov;
  iov.iov_base = data;
  iov.iov_len = data_size;

  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

#if __sal_os_linux
  flags |= MSG_NOSIGNAL;
#endif

  auto size = call(::recvmsg, error, handle, &msg, flags);
  if (!size && data_size)
  {
    error = make_error_code(std::errc::broken_pipe);
  }
  else if (size == -1)
  {
    size = 0;
  }
  else if (msg.msg_flags & MSG_TRUNC)
  {
    error.assign(EMSGSIZE, std::generic_category());
    size = 0;
  }
  return size;
}


size_t socket_t::receive_from (void *data, size_t data_size,
  void *address, size_t *address_size,
  message_flags_t flags,
  std::error_code &error) noexcept
{
  iovec iov;
  iov.iov_base = data;
  iov.iov_len = data_size;

  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_name = address;
  msg.msg_namelen = *address_size;

#if __sal_os_linux
  flags |= MSG_NOSIGNAL;
#endif

  auto size = call(::recvmsg, error, handle, &msg, flags);
  if (size >= 0)
  {
    if (msg.msg_flags & MSG_TRUNC)
    {
      error.assign(EMSGSIZE, std::generic_category());
      size = 0;
    }
    else
    {
      *address_size = msg.msg_namelen;
    }
  }
  else
  {
    size = 0;
  }
  return size;
}


size_t socket_t::send (const void *data, size_t data_size, message_flags_t flags,
  std::error_code &error) noexcept
{
  iovec iov;
  iov.iov_base = const_cast<void *>(data);
  iov.iov_len = data_size;

  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

#if __sal_os_linux
  flags |= MSG_NOSIGNAL;
#endif

  auto size = call(::sendmsg, error, handle, &msg, flags);
  if (size == -1)
  {
    size = 0;
  }
  return size;
}


size_t socket_t::send_to (const void *data, size_t data_size,
  const void *address, size_t address_size,
  message_flags_t flags,
  std::error_code &error) noexcept
{
  iovec iov;
  iov.iov_base = const_cast<void *>(data);
  iov.iov_len = data_size;

  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_name = const_cast<void *>(address);
  msg.msg_namelen = address_size;

#if __sal_os_linux
  flags |= MSG_NOSIGNAL;
#endif

  auto size = call(::sendmsg, error, handle, &msg, flags);
  if (size == -1)
  {
    size = 0;
  }
  return size;
}


void socket_t::shutdown (shutdown_t what, std::error_code &error) noexcept
{
  call(::shutdown, error, handle, static_cast<int>(what));
}


void socket_t::remote_endpoint (void *address, size_t *address_size,
  std::error_code &error) const noexcept
{
  auto size = static_cast<socklen_t>(*address_size);
  auto result = call(::getpeername,
    error,
    handle,
    static_cast<sockaddr *>(address), &size
  );
  if (result != -1)
  {
    *address_size = size;
  }
}


void socket_t::local_endpoint (void *address, size_t *address_size,
  std::error_code &error) const noexcept
{
  auto size = static_cast<socklen_t>(*address_size);
  auto result = call(::getsockname,
    error,
    handle,
    static_cast<sockaddr *>(address), &size
  );
  if (result != -1)
  {
    *address_size = size;
  }
}


void socket_t::get_opt (int level, int name, void *data, size_t *size,
  std::error_code &error) const noexcept
{
  auto data_size = static_cast<socklen_t>(*size);
  auto result = call(::getsockopt,
    error,
    handle,
    level, name,
    static_cast<char *>(data), &data_size
  );
  if (result != -1)
  {
    *size = data_size;
  }
}


void socket_t::set_opt (int level, int name, const void *data, size_t size,
  std::error_code &error) noexcept
{
  call(::setsockopt,
    error,
    handle,
    level, name,
    static_cast<const char *>(data), static_cast<socklen_t>(size)
  );
}


bool socket_t::non_blocking (std::error_code &error) const noexcept
{
  // on error, returned value is undefined
  return O_NONBLOCK & call(::fcntl, error, handle, F_GETFL, 0);
}


void socket_t::non_blocking (bool mode, std::error_code &error) noexcept
{
  int flags = call(::fcntl, error, handle, F_GETFL, 0);
  if (flags >= 0)
  {
    if (mode)
    {
      flags |= O_NONBLOCK;
    }
    else
    {
      flags &= ~O_NONBLOCK;
    }
    call(::fcntl, error, handle, F_SETFL, flags);
  }
}


size_t socket_t::available (std::error_code &error) const noexcept
{
  unsigned long value{};
  if (call(::ioctl, error, handle, FIONREAD, &value) == -1)
  {
    value = 0;
  }
  return value;
}


#endif // }}}1


}} // namespace net::__bits


__sal_end
