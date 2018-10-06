#pragma once

/**
 * \file sal/net/ip/resolver_base.hpp
 * Resolver types common ancestor
 */


#include <sal/config.hpp>
#include <sal/net/ip/__bits/inet.hpp>


__sal_begin


namespace net::ip {


/**
 * Common ancestor for different resolver types.
 */
class resolver_base_t
{
public:

  /// Resolver query parameters
  using flags_t = int;

  /**
   * POSIX macro AI_PASSIVE. Returned endpoints are intended for use as
   * locally bound socket endpoints.
   */
  static constexpr flags_t passive = AI_PASSIVE;

  /**
   * POSIX macro AI_CANONNAME. Determine the canonical name of the host
   * specified in query.
   */
  static constexpr flags_t canonical_name = AI_CANONNAME;

  /**
   * POSIX macro AI_NUMERICHOST. Host name should be treated as numeric string
   * defining and IPv4 or IPv6 address and no host name resolution should be
   * attempted.
   */
  static constexpr flags_t numeric_host = AI_NUMERICHOST;

  /**
   * POSIX macro AI_NUMERICSERV. Service name should be treated as a numeric
   * string defining a port numebr and no service name resolution should be
   * attempted.
   */
  static constexpr flags_t numeric_service = AI_NUMERICSERV;

  /**
   * POSIX macro AI_V4MAPPED. If the protocol is specified as an IPv6
   * protocol, return IPv4-mapped IPv6 address on finding no IPv6 address.
   */
  static constexpr flags_t v4_mapped = AI_V4MAPPED;

  /**
   * POSIX macro AI_ALL. If used with v4_mapped, return all matching IPv6 and
   * IPv4 addresses.
   */
  static constexpr flags_t all_matching = AI_ALL;

  /**
   * POSIX macro AI_ADDRCONFIG. Only return IPv4 addresses if a non-loopback
   * IPv4 address is configured for the system. Only return IPv4 addresses if
   * a non-loopback IPv4 address is configured for the system.
   */
  static constexpr flags_t address_configured = AI_ADDRCONFIG;


protected:

  ~resolver_base_t () noexcept
  {}
};


} // namespace net::ip


__sal_end
