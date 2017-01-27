#include <sal/net/__bits/socket.hpp>
#include <sal/net/error.hpp>

#if __sal_os_windows
  #include <mswsock.h>
#elif __sal_os_linux || __sal_os_darwin
  #include <fcntl.h>
  #include <poll.h>
  #include <sys/ioctl.h>
  #include <unistd.h>
#endif


__sal_begin


namespace net { namespace __bits {


namespace {

template <typename Result>
inline Result handle (Result result, std::error_code &error) noexcept
{
#if __sal_os_windows

  if (result == SOCKET_ERROR)
  {
    auto e = ::WSAGetLastError();
    if (e == WSAENOTSOCK)
    {
      e = WSAEBADF;
    }
    error.assign(e, std::system_category());
  }

#else

  if (result == -1)
  {
    error.assign(errno, std::generic_category());
  }

#endif

  return result;
}

} // namespace


void socket_t::open (int domain, int type, int protocol,
  std::error_code &error) noexcept
{
  // note: under Windows, socket() creates handles with overlapped attribute
  native_handle = handle(::socket(domain, type, protocol), error);

#if __sal_os_windows

  if (native_handle != SOCKET_ERROR)
  {
    // we handle immediate completion by deferring handling
    ::SetFileCompletionNotificationModes(
      reinterpret_cast<HANDLE>(native_handle),
      FILE_SKIP_COMPLETION_PORT_ON_SUCCESS |
      FILE_SKIP_SET_EVENT_ON_HANDLE
    );

    if (type == SOCK_DGRAM)
    {
      bool new_behaviour = false;
      DWORD ignored;
      ::WSAIoctl(native_handle,
        SIO_UDP_CONNRESET,
        &new_behaviour, sizeof(new_behaviour),
        nullptr, 0,
        &ignored,
        nullptr,
        nullptr
      );
    }
  }

#endif
}


void socket_t::close (std::error_code &error) noexcept
{
#if __sal_os_windows

  handle(::closesocket(native_handle), error);
  native_handle = invalid_socket;

#else

  for (;;)
  {
    if (handle(::close(native_handle), error) == 0 || errno != EINTR)
    {
      native_handle = invalid_socket;
      return;
    }
  }

#endif
}


void socket_t::bind (const void *address, size_t address_size,
  std::error_code &error) noexcept
{
  handle(
    ::bind(native_handle,
      static_cast<const sockaddr *>(address),
      static_cast<socklen_t>(address_size)
    ),
    error
  );
}


void socket_t::listen (int backlog, std::error_code &error) noexcept
{
  handle(::listen(native_handle, backlog), error);
}


native_socket_t socket_t::accept (void *address, size_t *address_size,
  bool enable_connection_aborted, std::error_code &error) noexcept
{
  socklen_t size{}, *size_p = nullptr;
  if (address_size)
  {
    size = static_cast<socklen_t>(*address_size);
    size_p = &size;
  }

retry:
  auto new_socket = handle(
    ::accept(native_handle, static_cast<sockaddr *>(address), size_p),
    error
  );

  if (new_socket != invalid_socket)
  {
    if (address_size)
    {
#if __sal_os_darwin
      // LCOV_EXCL_START
      // kernel bug: instead of ECONNABORTED, we might get size=0
      if (!size)
      {
        if (enable_connection_aborted)
        {
          error.assign(ECONNABORTED, std::generic_category());
          return native_socket_t{};
        }
        size = static_cast<socklen_t>(*address_size);
        goto retry;
      }
      // LCOV_EXCL_STOP
#endif
      *address_size = size;
    }
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

  return 0;
}


void socket_t::connect (const void *address, size_t address_size,
  std::error_code &error) noexcept
{
  handle(
    ::connect(native_handle,
      static_cast<const sockaddr *>(address),
      static_cast<socklen_t>(address_size)
    ),
    error
  );
}


bool socket_t::wait (wait_t what, int timeout_ms, std::error_code &error)
  noexcept
{
#if __sal_os_darwin || __sal_os_linux
  if (native_handle == invalid_socket)
  {
    error.assign(EBADF, std::generic_category());
    return false;
  }
#elif __sal_os_windows
  #define poll WSAPoll
#endif

  pollfd fd{};
  fd.fd = native_handle;
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
    handle(invalid_socket, error);
  }
  // LCOV_EXCL_STOP

  return false;
}

size_t socket_t::receive (void *data, size_t data_size, message_flags_t flags,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  WSABUF buf;
  buf.buf = static_cast<char *>(data);
  buf.len = static_cast<ULONG>(data_size);

  DWORD transferred{};
  DWORD recv_flags = flags;

  auto result = handle(
    ::WSARecv(native_handle,
      &buf, 1,
      &transferred,
      &recv_flags,
      nullptr,
      nullptr
    ),
    error
  );

  if (result != SOCKET_ERROR || error.value() == WSAESHUTDOWN)
  {
    if (transferred)
    {
      return transferred;
    }
    error = make_error_code(socket_errc_t::orderly_shutdown);
  }

  return 0;

#else

  iovec iov;
  iov.iov_base = data;
  iov.iov_len = data_size;

  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  auto size = handle(::recvmsg(native_handle, &msg, flags), error);
  if (!size)
  {
    error = make_error_code(socket_errc_t::orderly_shutdown);
  }
  else if (size == -1)
  {
    size = 0;
  }
  return size;

#endif
}


size_t socket_t::receive_from (void *data, size_t data_size,
  void *address, size_t *address_size,
  message_flags_t flags,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  WSABUF buf;
  buf.buf = static_cast<char *>(data);
  buf.len = static_cast<ULONG>(data_size);

  auto tmp_address_size = static_cast<INT>(*address_size);

  DWORD transferred{};
  DWORD recv_flags = flags;

  auto result = handle(
    ::WSARecvFrom(native_handle,
      &buf, 1,
      &transferred,
      &recv_flags,
      static_cast<sockaddr *>(address),
      (address ? &tmp_address_size : nullptr),
      nullptr,
      nullptr
    ),
    error
  );

  if (result != SOCKET_ERROR)
  {
    *address_size = tmp_address_size;
    return transferred;
  }

  return 0;

#else

  iovec iov;
  iov.iov_base = data;
  iov.iov_len = data_size;

  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_name = address;
  msg.msg_namelen = *address_size;

  auto size = handle(::recvmsg(native_handle, &msg, flags), error);
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

#endif
}


size_t socket_t::send (const void *data, size_t data_size, message_flags_t flags,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  WSABUF buf;
  buf.buf = static_cast<char *>(const_cast<void *>(data));
  buf.len = static_cast<ULONG>(data_size);

  DWORD transferred{};

  auto result = handle(
    ::WSASend(native_handle,
      &buf, 1,
      &transferred,
      flags,
      nullptr,
      nullptr
    ),
    error
  );

  if (result != SOCKET_ERROR)
  {
    return transferred;
  }
  else if (error.value() == WSAESHUTDOWN)
  {
    error.assign(EPIPE, std::generic_category());
  }

  return 0;

#else

  iovec iov;
  iov.iov_base = const_cast<void *>(data);
  iov.iov_len = data_size;

  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  auto size = handle(::sendmsg(native_handle, &msg, flags), error);
  if (size == -1)
  {
    size = 0;
  }
  return size;

#endif
}


size_t socket_t::send_to (const void *data, size_t data_size,
  const void *address, size_t address_size,
  message_flags_t flags,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  WSABUF buf;
  buf.buf = static_cast<char *>(const_cast<void *>(data));
  buf.len = static_cast<ULONG>(data_size);

  DWORD transferred{};

  auto result = handle(
    ::WSASendTo(native_handle,
      &buf, 1,
      &transferred,
      flags,
      static_cast<const sockaddr *>(address),
      static_cast<int>(address_size),
      nullptr,
      nullptr
    ),
    error
  );

  if (result != SOCKET_ERROR)
  {
    return transferred;
  }
  else if (error.value() == WSAENOTCONN)
  {
    error.assign(WSAEDESTADDRREQ, std::system_category());
  }

  return 0;

#else

  iovec iov;
  iov.iov_base = const_cast<void *>(data);
  iov.iov_len = data_size;

  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_name = const_cast<void *>(address);
  msg.msg_namelen = address_size;

  auto size = handle(::sendmsg(native_handle, &msg, flags), error);
  if (size == -1)
  {
    size = 0;
  }
  return size;

#endif
}


void socket_t::shutdown (int what, std::error_code &error) noexcept
{
  handle(::shutdown(native_handle, what), error);
}


void socket_t::remote_endpoint (void *address, size_t *address_size,
  std::error_code &error) const noexcept
{
  auto size = static_cast<socklen_t>(*address_size);
  auto result = handle(
    ::getpeername(native_handle, static_cast<sockaddr *>(address), &size),
    error
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
  auto result = handle(
    ::getsockname(native_handle, static_cast<sockaddr *>(address), &size),
    error
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
  auto result = handle(
    ::getsockopt(native_handle, level, name,
      static_cast<char *>(data), &data_size
    ),
    error
  );
  if (result != -1)
  {
    *size = data_size;
  }
}


void socket_t::set_opt (int level, int name, const void *data, size_t size,
  std::error_code &error) noexcept
{
  handle(
    ::setsockopt(native_handle, level, name,
      static_cast<const char *>(data), static_cast<socklen_t>(size)
    ),
    error
  );
}


bool socket_t::non_blocking (std::error_code &error) const noexcept
{
#if __sal_os_windows

  error.assign(WSAEOPNOTSUPP, std::system_category());
  return true;

#else

  // on error, returned value is undefined
  return O_NONBLOCK & handle(::fcntl(native_handle, F_GETFL, 0), error);

#endif
}


void socket_t::non_blocking (bool mode, std::error_code &error) noexcept
{
#if __sal_os_windows

  unsigned long arg = mode ? 1 : 0;
  handle(::ioctlsocket(native_handle, FIONBIO, &arg), error);

#else

  int flags = handle(::fcntl(native_handle, F_GETFL, 0), error);
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
    if (::fcntl(native_handle, F_SETFL, flags) != -1)
    {
      return;
    }
  }

#endif
}


size_t socket_t::available (std::error_code &error) const noexcept
{
  unsigned long value{};

#if __sal_os_windows

  if (handle(::ioctlsocket(native_handle, FIONBIO, &value), error) == SOCKET_ERROR)
  {
    value = 0;
  }

#else

  if (handle(::ioctl(native_handle, FIONREAD, &value), error) == -1)
  {
    value = 0;
  }

#endif

  return value;
}


bool socket_t::start (io_buf_t *io_buf,
  void *data, size_t data_size,
  message_flags_t flags,
  async_receive_from_t &op) noexcept
{
#if __sal_os_windows

  WSABUF wsabuf;
  wsabuf.buf = static_cast<char *>(data);
  wsabuf.len = static_cast<DWORD>(data_size);

  DWORD transferred{}, flags_ = flags;
  auto result = ::WSARecvFrom(native_handle,
    &wsabuf, 1,
    &transferred,
    &flags_,
    reinterpret_cast<sockaddr *>(&op.endpoint_),
    &op.endpoint_size_,
    static_cast<OVERLAPPED *>(io_buf),
    nullptr
  );

  if (result == 0)
  {
    // completed immediately
    op.transferred_ = transferred;
    return true;
  }

  auto e = ::WSAGetLastError();
  if (e == WSA_IO_PENDING)
  {
    // will be completed later, OS now owns data
    return false;
  }

  // failed, caller still owns data
  op.error_.assign(e, std::system_category());
  return true;

#else

  (void)io_buf;
  (void)data;
  (void)data_size;
  (void)flags;
  (void)op;

  return true;

#endif
}


bool socket_t::start (io_buf_t *io_buf, const void *data, size_t data_size,
  const void *address, size_t address_size,
  message_flags_t flags,
  async_send_to_t &op) noexcept
{
#if __sal_os_windows

  WSABUF wsabuf;
  wsabuf.buf = static_cast<char *>(const_cast<void *>(data));
  wsabuf.len = static_cast<DWORD>(data_size);

  DWORD transferred{};
  auto result = ::WSASendTo(native_handle,
    &wsabuf, 1,
    &transferred,
    flags,
    static_cast<const sockaddr *>(address),
    static_cast<int>(address_size),
    static_cast<OVERLAPPED *>(io_buf),
    nullptr
  );

  if (result == 0)
  {
    // completed immediately
    op.transferred_ = transferred;
    return true;
  }

  auto e = ::WSAGetLastError();
  if (e == WSA_IO_PENDING)
  {
    // will be completed later, OS now owns data
    return false;
  }

  // failed, caller still owns data
  op.error_.assign(e, std::system_category());
  return true;

#else

  (void)io_buf;
  (void)data;
  (void)data_size;
  (void)address;
  (void)address_size;
  (void)flags;
  (void)op;

  return true;

#endif
}


}} // namespace net::__bits


__sal_end
