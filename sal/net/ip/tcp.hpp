#pragma once

/**
 * \file sal/net/ip/tcp.hpp
 * TCP protocol
 */


#include <sal/config.hpp>
#include <sal/net/ip/__bits/inet.hpp>
#include <sal/net/ip/basic_endpoint.hpp>
#include <sal/net/ip/basic_resolver.hpp>
#include <sal/net/basic_stream_socket.hpp>
#include <sal/net/basic_socket_acceptor.hpp>
#include <sal/memory_writer.hpp>
#include <ostream>


__sal_begin


namespace net::ip {


/**
 * This class encapsulates types and flags necessary for TCP sockets.
 */
class tcp_t
{
public:

  /// TCP socket endpoint
  using endpoint_t = basic_endpoint_t<tcp_t>;

  /// TCP endpoint resolver
  using resolver_t = basic_resolver_t<tcp_t>;

  /// TCP stream socket
  using socket_t = basic_stream_socket_t<tcp_t>;

  /// TCP stream acceptor
  using acceptor_t = basic_socket_acceptor_t<tcp_t>;


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
   * TCP/IPv4 internet protocol.
   */
  static const tcp_t v4;


  /**
   * TCP/IPv6 internet protocol.
   */
  static const tcp_t v6;


  /**
   * Return option setter for TCP_NODELAY. Sets flag whether TCP socket will
   * avoid coalescing of small segments (i.e. disables the Nagle algorithm)
   */
  static auto no_delay (bool value) noexcept
    -> ::sal::net::__bits::socket_option_setter_t<IPPROTO_TCP, TCP_NODELAY, bool>
  {
    return value;
  }


  /**
   * Return option getter for TCP_NODELAY. Queries flag whether TCP socket
   * will avoid coalescing of small segments (i.e. disables the Nagle
   * algorithm).
   */
  static auto no_delay (bool *value) noexcept
    -> ::sal::net::__bits::socket_option_getter_t<IPPROTO_TCP, TCP_NODELAY, bool>
  {
    return value;
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


/**
 * Insert human readable \a protocol representation into \a writer.
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const tcp_t &protocol) noexcept
{
  return protocol.family() == AF_INET
    ? writer.print("AF_INET")
    : writer.print("AF_INET6")
  ;
}


/**
 * Insert human readable \a protocol into std::ostream \a os
 */
inline std::ostream &operator<< (std::ostream &os, const tcp_t &protocol)
{
  char_array_t<sizeof("AF_INET6")> buf;
  buf << protocol;
  return (os << buf.c_str());
}


} // namespace net::ip


__sal_end
