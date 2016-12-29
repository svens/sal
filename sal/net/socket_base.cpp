#include <sal/net/socket_base.hpp>

#if !__sal_os_windows
  #include <unistd.h>
#endif


__sal_begin

namespace net {


constexpr socket_base_t::native_handle_t socket_base_t::no_handle;
constexpr int socket_base_t::max_listen_connections;


namespace {


inline void get_last (std::error_code &error, bool align_with_posix=true)
  noexcept
{
#if __sal_os_windows

  auto e = ::WSAGetLastError();
  if (align_with_posix && e == WSAENOTSOCK)
  {
    e = WSAEBADF;
  }
  error.assign(e, std::system_category());

#else

  (void)align_with_posix;
  error.assign(errno, std::generic_category());

#endif
}


} // namespace


socket_base_t::native_handle_t socket_base_t::open (int domain,
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


void socket_base_t::close (native_handle_t handle,
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


void socket_base_t::get_opt (native_handle_t handle,
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


void socket_base_t::set_opt (native_handle_t handle,
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


} // namespace net

__sal_end
