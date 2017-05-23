#include <sal/net/__bits/socket.hpp>
#include <sal/net/error.hpp>
#include <mutex>

#if __sal_os_windows
  #include <mswsock.h>
#elif __sal_os_darwin || __sal_os_linux
  #include <fcntl.h>
  #include <poll.h>
  #include <signal.h>
  #include <sys/ioctl.h>
  #include <unistd.h>
#endif


__sal_begin


namespace net {


namespace {


#if __sal_os_windows


struct lib_t
{
  lib_t () noexcept
  {
    setup();
  }

  ~lib_t () noexcept
  {
    cleanup();
  }

  static std::error_code setup_result;
  static lib_t lib;

  static void setup () noexcept;
  static void cleanup () noexcept;
};

std::error_code lib_t::setup_result{};
lib_t lib_t::lib;


void internal_setup (std::error_code &result) noexcept
{

  WSADATA wsa;
  result.assign(
    ::WSAStartup(MAKEWORD(2, 2), &wsa),
    std::system_category()
  );
}


void lib_t::setup () noexcept
{
  static std::once_flag flag;
  std::call_once(flag, &internal_setup, std::ref(setup_result));
}


void lib_t::cleanup () noexcept
{
  ::WSACleanup();
}


#endif


} // namespace


const std::error_code &init () noexcept
{
#if __sal_os_windows

  lib_t::setup();
  return lib_t::setup_result;

#else

  static const std::error_code no_error{};
  return no_error;

#endif
}


namespace __bits {


namespace {


template <typename T>
inline T check_call (T result, std::error_code &error) noexcept
{
#if __sal_os_windows

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

#else

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

#endif

  return result;
}

#define call(error,func,...) check_call(func(__VA_ARGS__), error)


} // namespace


void socket_t::open (int domain, int type, int protocol,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  handle = call(error, ::WSASocketW,
    domain,
    type,
    protocol,
    nullptr,
    0,
    WSA_FLAG_OVERLAPPED
  );

  if (handle != SOCKET_ERROR)
  {
    // we handle immediate completion by deferring handling
    ::SetFileCompletionNotificationModes(
      reinterpret_cast<HANDLE>(handle),
      FILE_SKIP_COMPLETION_PORT_ON_SUCCESS |
      FILE_SKIP_SET_EVENT_ON_HANDLE
    );

    if (type == SOCK_DGRAM)
    {
      bool new_behaviour = false;
      DWORD ignored;
      ::WSAIoctl(handle,
        SIO_UDP_CONNRESET,
        &new_behaviour, sizeof(new_behaviour),
        nullptr, 0,
        &ignored,
        nullptr,
        nullptr
      );
    }
  }

#else

  handle = call(error, ::socket, domain, type, protocol);

#if __sal_os_darwin
  if (handle != invalid)
  {
    int optval = 1;
    (void)::setsockopt(handle, SOL_SOCKET, SO_NOSIGPIPE,
      &optval, sizeof(optval)
    );
  }
#endif

#endif
}


void socket_t::close (std::error_code &error) noexcept
{
#if __sal_os_windows

  call(error, ::closesocket, handle);
  handle = invalid;

#else

  for (;;)
  {
    if (call(error, ::close, handle) == 0 || errno != EINTR)
    {
      handle = invalid;
      return;
    }
  }

#endif
}


void socket_t::bind (const void *address, size_t address_size,
  std::error_code &error) noexcept
{
  call(error, ::bind,
    handle,
    static_cast<const sockaddr *>(address),
    static_cast<socklen_t>(address_size)
  );
}


void socket_t::listen (int backlog, std::error_code &error) noexcept
{
  call(error, ::listen, handle, backlog);
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
  auto new_socket = call(error, ::accept,
    handle,
    static_cast<sockaddr *>(address),
    size_p
  );

  if (new_socket != invalid)
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
          return invalid;
        }
        size = static_cast<socklen_t>(*address_size);
        goto retry;
      }
      // LCOV_EXCL_STOP
#endif
      *address_size = size;
    }

#if __sal_os_darwin
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

  return 0;
}


void socket_t::connect (const void *address, size_t address_size,
  std::error_code &error) noexcept
{
  call(error, ::connect,
    handle,
    static_cast<const sockaddr *>(address),
    static_cast<socklen_t>(address_size)
  );
}


bool socket_t::wait (wait_t what, int timeout_ms, std::error_code &error)
  noexcept
{
#if __sal_os_darwin || __sal_os_linux
  if (handle == invalid)
  {
    error.assign(EBADF, std::generic_category());
    return false;
  }
#elif __sal_os_windows
  #define poll WSAPoll
#endif

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
#if __sal_os_windows

  WSABUF buf;
  buf.buf = static_cast<char *>(data);
  buf.len = static_cast<ULONG>(data_size);

  DWORD transferred{};
  DWORD recv_flags = flags;

  auto result = call(error, ::WSARecv,
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

#else

  iovec iov;
  iov.iov_base = data;
  iov.iov_len = data_size;

  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

#if __sal_os_linux
  flags |= MSG_NOSIGNAL;
#endif

  auto size = call(error, ::recvmsg, handle, &msg, flags);
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

  auto result = call(error, ::WSARecvFrom,
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

#else

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

  auto size = call(error, ::recvmsg, handle, &msg, flags);
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

  auto result = call(error, ::WSASend,
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

#else

  iovec iov;
  iov.iov_base = const_cast<void *>(data);
  iov.iov_len = data_size;

  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

#if __sal_os_linux
  flags |= MSG_NOSIGNAL;
#endif

  auto size = call(error, ::sendmsg, handle, &msg, flags);
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

  auto result = call(error, ::WSASendTo,
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

#else

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

  auto size = call(error, ::sendmsg, handle, &msg, flags);
  if (size == -1)
  {
    size = 0;
  }
  return size;

#endif
}


void socket_t::shutdown (shutdown_t what, std::error_code &error) noexcept
{
  call(error, ::shutdown, handle, static_cast<int>(what));
}


void socket_t::remote_endpoint (void *address, size_t *address_size,
  std::error_code &error) const noexcept
{
  auto size = static_cast<socklen_t>(*address_size);
  auto result = call(error, ::getpeername,
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
  auto result = call(error, ::getsockname,
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
  auto result = call(error, ::getsockopt,
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
  call(error, ::setsockopt,
    handle,
    level, name,
    static_cast<const char *>(data), static_cast<socklen_t>(size)
  );
}


bool socket_t::non_blocking (std::error_code &error) const noexcept
{
#if __sal_os_windows

  error.assign(WSAEOPNOTSUPP, std::system_category());
  return true;

#else

  // on error, returned value is undefined
  return O_NONBLOCK & call(error, ::fcntl, handle, F_GETFL, 0);

#endif
}


void socket_t::non_blocking (bool mode, std::error_code &error) noexcept
{
#if __sal_os_windows

  unsigned long arg = mode ? 1 : 0;
  call(error, ::ioctlsocket, handle, FIONBIO, &arg);

#else

  int flags = call(error, ::fcntl, handle, F_GETFL, 0);
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
    call(error, ::fcntl, handle, F_SETFL, flags);
  }

#endif
}


size_t socket_t::available (std::error_code &error) const noexcept
{
  unsigned long value{};

#if __sal_os_windows

  if (call(error, ::ioctlsocket, handle, FIONBIO, &value) == SOCKET_ERROR)
  {
    value = 0;
  }

#else

  if (call(error, ::ioctl, handle, FIONREAD, &value) == -1)
  {
    value = 0;
  }

#endif

  return value;
}


}} // namespace net::__bits


__sal_end
