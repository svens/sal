#include <sal/net/ip/address_v4.hpp>


__sal_begin


namespace net::ip {

const address_v4_t address_v4_t::any{INADDR_ANY};
const address_v4_t address_v4_t::loopback{INADDR_LOOPBACK};
const address_v4_t address_v4_t::broadcast{INADDR_BROADCAST};

} // namespace net::ip


__sal_end
