#include <sal/net/async/__bits/async.hpp>


__sal_begin


namespace net::async::__bits {


#if __sal_os_windows //{{{1


service_t::service_t ()
{ }


service_t::~service_t () noexcept
{ }


handler_t::handler_t (service_ptr service,
    socket_t &socket,
    std::error_code &error) noexcept
  : service(service)
{
  (void)socket;
  error.clear();
}


#elif __sal_os_linux || __sal_os_macos //{{{1


service_t::service_t ()
{ }


service_t::~service_t () noexcept
{ }


handler_t::handler_t (service_ptr service,
    socket_t &socket,
    std::error_code &error) noexcept
  : service(service)
{
  (void)socket;
  error.clear();
}


#endif //}}}1


} // namespace net::async::__bits


__sal_end
