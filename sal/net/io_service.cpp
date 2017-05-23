#include <sal/net/io_service.hpp>


__sal_begin


namespace net {


void io_service_t::associate (__bits::socket_t &socket, std::error_code &error)
  noexcept
{
  (void)socket;
  (void)error;
}


} // namespace net


__sal_end
