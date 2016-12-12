#include <sal/net/ip/__bits/platform.hpp>


__sal_begin


namespace net { namespace ip { namespace __bits {


lib_t lib_t::instance;


lib_t::lib_t ()
{
#if 0
#if __sal_os_windows
  static WSADATA wsa;
  WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
#endif
}


}}} // namespace net::ip::__bits


__sal_end
