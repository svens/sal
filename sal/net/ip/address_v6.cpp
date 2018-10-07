#include <sal/net/ip/address_v6.hpp>


__sal_begin


namespace net::ip {

const address_v6_t address_v6_t::any{in6addr_any};
const address_v6_t address_v6_t::loopback{in6addr_loopback};

} // namespace net::ip


__sal_end
