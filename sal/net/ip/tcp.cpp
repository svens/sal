#include <sal/net/ip/tcp.hpp>


__sal_begin


namespace net::ip {

const tcp_t tcp_t::v4{AF_INET};
const tcp_t tcp_t::v6{AF_INET6};

} // namespace net::ip


__sal_end
