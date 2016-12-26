#pragma once

/**
 * \file sal/net/ip/tcp.hpp
 * TCP protocol
 */


#include <sal/config.hpp>
#include <sal/net/ip/endpoint.hpp>
#include <sal/net/ip/resolver.hpp>


__sal_begin


namespace net { namespace ip {


/**
 * This class encapsulates types and flags necessary for TCP sockets.
 */
class tcp_t
{
public:

  /**
   * TCP socket endpoint
   */
  using endpoint_t = basic_endpoint_t<tcp_t>;

  /**
   * TCP endpoint resolver
   */
  using resolver_t = basic_resolver_t<tcp_t>;


  tcp_t () = delete;


  /**
   * Return value suitable passing as domain argument for socket(3)
   */
  constexpr int family () const noexcept
  {
    return family_;
  }


  /**
   * Return value suitable passing as type argument for socket(3)
   */
  constexpr int type () const noexcept
  {
    return SOCK_STREAM;
  }


  /**
   * Return value suitable passing as protocol argument for socket(3)
   */
  constexpr int protocol () const noexcept
  {
    return IPPROTO_TCP;
  }


  /**
   * Return TCP/IPv4 internet protocol instance.
   */
  static constexpr tcp_t v4 () noexcept
  {
    return tcp_t{AF_INET};
  }


  /**
   * Return TCP/IPv6 internet protocol instance.
   */
  static constexpr tcp_t v6 () noexcept
  {
    return tcp_t{AF_INET6};
  }


private:

  int family_;

  constexpr explicit tcp_t (int family) noexcept
    : family_(family)
  {}

  friend class basic_endpoint_t<tcp_t>;
};


/**
 * Return true if \a a and \a b families are same.
 */
constexpr bool operator== (const tcp_t &a, const tcp_t &b) noexcept
{
  return a.family() == b.family();
}


/**
 * Return true if \a a and \a b families are different.
 */
constexpr bool operator!= (const tcp_t &a, const tcp_t &b) noexcept
{
  return !(a == b);
}


}} // namespace net::ip


__sal_end
