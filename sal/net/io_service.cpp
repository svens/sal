#include <sal/net/io_service.hpp>


__sal_begin


namespace net {


void io_service_t::register_socket (const socket_base_t &socket,
  uintptr_t socket_data, std::error_code &error) noexcept
{
  (void)socket;
  (void)socket_data;
  (void)error;
}


} // namespace net


__sal_end
