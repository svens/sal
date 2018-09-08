#include <sal/net/async/__bits/async.hpp>


__sal_begin


namespace net::async::__bits {


#if __sal_os_windows //{{{1


service_t::service_t ()
{ }


service_t::~service_t () noexcept
{ }


#elif __sal_os_linux || __sal_os_macos //{{{1


service_t::service_t ()
{ }


service_t::~service_t () noexcept
{ }


#endif //}}}1


} // namespace net::async::__bits


__sal_end
