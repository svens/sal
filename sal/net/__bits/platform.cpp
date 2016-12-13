#include <sal/net/__bits/platform.hpp>
#include <mutex>


__sal_begin


namespace net { namespace __bits {


lib_t lib_t::instance;


namespace {

void internal_startup () noexcept
{
#if __sal_os_windows
  static WSADATA wsa;
  WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

} // namespace


void lib_t::startup () noexcept
{
  static std::once_flag initialized;
  std::call_once(initialized, &internal_startup);
}


lib_t::~lib_t () noexcept
{
#if __sal_os_windows
  WSACleanup();
#endif
}


}} // namespace net::__bits


__sal_end
