#include <sal/net/__bits/platform.hpp>
#include <sal/net/error.hpp>

#if __sal_os_windows
  #include <mutex>
#else
  #include <fcntl.h>
  #include <poll.h>
  #include <sys/ioctl.h>
  #include <unistd.h>
#endif


__sal_begin


namespace net {


#if __sal_os_windows

namespace {


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
  std::call_once(flag, &internal_setup, setup_result);
}


void lib_t::cleanup () noexcept
{
  ::WSACleanup();
}


} // namespace

#endif


const std::error_code &init () noexcept
{
#if __sal_os_windows
  lib_t::setup();
  return lib_t::setup_result;
#else
  static std::error_code result{};
  return result;
#endif
}


namespace __bits {


namespace {

inline void get_last (std::error_code &error, bool align_with_posix=true)
  noexcept
{
#if __sal_os_windows

  auto e = ::WSAGetLastError();
  if (align_with_posix)
  {
    if (e == WSAENOTSOCK)
    {
      e = WSAEBADF;
    }
  }
  error.assign(e, std::system_category());

#else

  (void)align_with_posix;
  error.assign(errno, std::generic_category());

#endif
}

} // namespace


native_handle_t open (int domain,
  int type,
  int protocol,
  std::error_code &error) noexcept
{
  auto handle = ::socket(domain, type, protocol);
  // ignore error handling, API doesn't let trigger invalid case
  // LCOV_EXCL_START
  if (handle == -1)
  {
    get_last(error, false);
  }
  // LCOV_EXCL_STOP
  return handle;
}


void close (native_handle_t handle,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  if (::closesocket(handle) == 0)
  {
    return;
  }

#else

  for (errno = EINTR;  errno == EINTR;  /**/)
  {
    if (::close(handle) == 0)
    {
      return;
    }
  }

#endif

  get_last(error);
}


void get_opt (native_handle_t handle,
  int level,
  int name,
  void *data,
  socklen_t *size,
  std::error_code &error) noexcept
{
  if (::getsockopt(handle, level, name, reinterpret_cast<char *>(data), size))
  {
    get_last(error);
  }
}


void set_opt (native_handle_t handle,
  int level,
  int name,
  const void *data,
  socklen_t size,
  std::error_code &error) noexcept
{
  if (::setsockopt(handle, level, name, reinterpret_cast<const char *>(data), size))
  {
    get_last(error);
  }
}


bool non_blocking (native_handle_t handle, std::error_code &error) noexcept
{
#if __sal_os_windows

  (void)handle;
  error.assign(WSAEOPNOTSUPP, std::system_category());
  return true;

#else

  auto flags = ::fcntl(handle, F_GETFL, 0);
  if (flags == -1)
  {
    get_last(error);
  }

  // it'll be unusable value
  return flags & O_NONBLOCK;

#endif
}


void non_blocking (native_handle_t handle,
  bool mode,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  unsigned long arg = mode ? 1 : 0;
  if (::ioctlsocket(handle, FIONBIO, &arg) != SOCKET_ERROR)
  {
    return;
  }

#else

  int flags = ::fcntl(handle, F_GETFL, 0);
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
    if (::fcntl(handle, F_SETFL, flags) != -1)
    {
      return;
    }
  }

#endif

  get_last(error);
}


size_t available (native_handle_t handle, std::error_code &error) noexcept
{
  unsigned long value{};

#if __sal_os_windows

  if (::ioctlsocket(handle, FIONBIO, &value) != SOCKET_ERROR)
  {
    return value;
  }

#else

  if (::ioctl(handle, FIONREAD, &value) != -1)
  {
    return value;
  }

#endif

  get_last(error);
  return 0U;
}


void bind (native_handle_t handle,
  const void *address, size_t address_size,
  std::error_code &error) noexcept
{
  if (::bind(handle,
      static_cast<const sockaddr *>(address),
      static_cast<socklen_t>(address_size)) == -1)
  {
    get_last(error);
  }
}


void listen (native_handle_t handle, int backlog, std::error_code &error)
  noexcept
{
  if (::listen(handle, backlog) == -1)
  {
    get_last(error);
  }
}


native_handle_t accept (native_handle_t handle,
  void *address,
  size_t *address_size,
  bool enable_connection_aborted,
  std::error_code &error) noexcept
{
  auto size = address ? static_cast<socklen_t>(*address_size) : 0;
  for (;;)
  {
    auto h = ::accept(handle, static_cast<sockaddr *>(address), &size);
    if (h == -1)
    {
      if (errno == ECONNABORTED && !enable_connection_aborted)
      {
        continue;
      }
      get_last(error);
    }
    else if (address_size)
    {
      *address_size = size;
    }
    return h;
  }
}


void connect (native_handle_t handle,
  const void *address, size_t address_size,
  std::error_code &error) noexcept
{
  if (::connect(handle,
      static_cast<const sockaddr *>(address),
      static_cast<socklen_t>(address_size)) == -1)
  {
    get_last(error);
  }
}


void shutdown (native_handle_t handle, int what, std::error_code &error)
  noexcept
{
  if (::shutdown(handle, what) == -1)
  {
    get_last(error);
  }
}


bool wait (native_handle_t handle, wait_t what, int timeout_ms,
  std::error_code &error) noexcept
{
#if __sal_os_darwin || __sal_os_linux
  if (handle == invalid_socket)
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
    get_last(error);
  }
  // LCOV_EXCL_STOP

  return false;
}


void local_endpoint (native_handle_t handle,
  void *address, size_t *address_size,
  std::error_code &error) noexcept
{
  auto size = static_cast<socklen_t>(*address_size);
  if (::getsockname(handle, static_cast<sockaddr *>(address), &size) != -1)
  {
    *address_size = size;
  }
  else
  {
    get_last(error);
  }
}


void remote_endpoint (native_handle_t handle,
  void *address, size_t *address_size,
  std::error_code &error) noexcept
{
  auto size = static_cast<socklen_t>(*address_size);
  if (::getpeername(handle, static_cast<sockaddr *>(address), &size) != -1)
  {
    *address_size = size;
  }
  else
  {
    get_last(error);
  }
}


size_t recv_from (native_handle_t handle,
  char *data, size_t data_size,
  void *address, size_t *address_size,
  int flags,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  auto _address_size = static_cast<socklen_t>(*address_size);
  auto size = ::recvfrom(handle,
    data, static_cast<int>(data_size),
    flags,
    static_cast<sockaddr *>(address), (address ? &_address_size : nullptr)
  );
  if (size >= 0)
  {
    *address_size = _address_size;
  }
  else
  {
    get_last(error);
    size = 0;
  }
  return size;

#else

  iovec iov;
  iov.iov_base = data;
  iov.iov_len = data_size;

  msghdr msg;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_name = address;
  msg.msg_namelen = *address_size;
  msg.msg_control = nullptr;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  auto size = ::recvmsg(handle, &msg, flags);
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
    get_last(error);
    size = 0;
  }
  return size;

#endif
}


size_t send_to (native_handle_t handle,
  const char *data, size_t data_size,
  const void *address, size_t address_size,
  int flags,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  auto size = ::sendto(handle,
    data, static_cast<int>(data_size),
    flags,
    static_cast<const sockaddr *>(address), static_cast<socklen_t>(address_size)
  );
  if (size == SOCKET_ERROR)
  {
    if (::WSAGetLastError() == WSAENOTCONN)
    {
      ::WSASetLastError(WSAEDESTADDRREQ);
    }
    get_last(error);
    size = 0;
  }
  return size;

#else

  iovec iov;
  iov.iov_base = const_cast<char *>(data);
  iov.iov_len = data_size;

  msghdr msg;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_name = const_cast<void *>(address);
  msg.msg_namelen = address_size;
  msg.msg_control = nullptr;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  auto size = ::sendmsg(handle, &msg, flags);
  if (size == -1)
  {
    get_last(error);
    size = 0;
  }
  return size;

#endif
}


size_t recv (native_handle_t handle,
  char *data, size_t data_size,
  int flags,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  auto size = ::recv(handle, data, static_cast<int>(data_size), flags);

#else

  iovec iov;
  iov.iov_base = data;
  iov.iov_len = data_size;

  msghdr msg;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_name = nullptr;
  msg.msg_namelen = 0;
  msg.msg_control = nullptr;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  auto size = ::recvmsg(handle, &msg, flags);

#endif

  if (size == 0)
  {
    error = make_error_code(socket_errc_t::orderly_shutdown);
  }
  else if (size == -1)
  {
    get_last(error);
    size = 0;
  }
  return size;
}


size_t send (native_handle_t handle,
  const char *data, size_t data_size,
  int flags,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  auto size = ::send(handle, data, static_cast<int>(data_size), flags);

#else

  iovec iov;
  iov.iov_base = const_cast<char *>(data);
  iov.iov_len = data_size;

  msghdr msg;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_name = nullptr;
  msg.msg_namelen = 0;
  msg.msg_control = nullptr;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  auto size = ::sendmsg(handle, &msg, flags);

#endif

  if (size == -1)
  {
    get_last(error);
    size = 0;
  }
  return size;
}


} // namespace __bits


} // namespace net


__sal_end
