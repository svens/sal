#include <sal/net/ip/udp.hpp>


__sal_begin


namespace net::ip {

const udp_t udp_t::v4{AF_INET};
const udp_t udp_t::v6{AF_INET6};

} // namespace net::ip


__sal_end
