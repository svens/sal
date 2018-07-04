#pragma once

/**
 * \file sal/net/ip/udp.hpp
 * UDP protocol
 */


#include <sal/config.hpp>
#include <sal/net/ip/__bits/inet.hpp>
#include <sal/net/ip/basic_endpoint.hpp>
#include <sal/net/ip/basic_resolver.hpp>
#include <sal/net/basic_datagram_socket.hpp>
#include <sal/net/async/basic_datagram_socket.hpp>
#include <sal/memory_writer.hpp>
#include <ostream>


__sal_begin


namespace net { namespace ip {


/**
 * This class encapsulates types and flags necessary for UDP sockets.
 */
class udp_t
{
public:

  /// UDP socket endpoint
  using endpoint_t = basic_endpoint_t<udp_t>;

  /// UDP endpoint resolver
  using resolver_t = basic_resolver_t<udp_t>;

  /// UDP datagram socket
  using socket_t = basic_datagram_socket_t<udp_t>;

  // Asynchronous UDP datagram socket
  using async_socket_t = async::basic_datagram_socket_t<udp_t>;


  udp_t () = delete;


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
    return SOCK_DGRAM;
  }


  /**
   * Return value suitable passing as protocol argument for socket(3)
   */
  constexpr int protocol () const noexcept
  {
    return IPPROTO_UDP;
  }


  /**
   * Return UDP/IPv4 internet protocol instance.
   */
  static constexpr udp_t v4 () noexcept
  {
    return udp_t{AF_INET};
  }


  /**
   * Return UDP/IPv6 internet protocol instance.
   */
  static constexpr udp_t v6 () noexcept
  {
    return udp_t{AF_INET6};
  }


private:

  int family_;

  constexpr explicit udp_t (int family) noexcept
    : family_(family)
  {}

  friend class basic_endpoint_t<udp_t>;
};


/**
 * Return true if \a a and \a b families are same.
 */
constexpr bool operator== (const udp_t &a, const udp_t &b) noexcept
{
  return a.family() == b.family();
}


/**
 * Return true if \a a and \a b families are different.
 */
constexpr bool operator!= (const udp_t &a, const udp_t &b) noexcept
{
  return !(a == b);
}


/**
 * Insert human readable \a protocol representation into \a writer.
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const udp_t &protocol) noexcept
{
  return protocol.family() == AF_INET
    ? writer.print("AF_INET")
    : writer.print("AF_INET6")
  ;
}


/**
 * Insert human readable \a protocol into std::ostream \a os
 */
inline std::ostream &operator<< (std::ostream &os, const udp_t &protocol)
{
  char_array_t<sizeof("AF_INET6")> buf;
  buf << protocol;
  return (os << buf.c_str());
}


}} // namespace net::ip


__sal_end
