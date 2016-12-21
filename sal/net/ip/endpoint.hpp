#pragma once

/**
 * \file sal/net/ip/endpoint.hpp
 * IP endpoint (address/port pair)
 */


#include <sal/config.hpp>
#include <sal/net/ip/address.hpp>
#include <sal/memory_writer.hpp>
#include <sal/net/error.hpp>
#include <ostream>


__sal_begin


namespace net { namespace ip {


/**
 * Endpoint represents a protocol-specific endpoint. It consists of an IP
 * address and port number. Endpoints are used to identify sources and
 * destinations for socket connections and datagrams.
 */
template <typename Protocol>
class basic_endpoint_t
{
public:

  /**
   * Endpoint's internet protocol
   */
  using protocol_t = Protocol;


  /**
   * Construct endpoint with unspecified address and port 0.
   */
  constexpr basic_endpoint_t () noexcept
  {
    addr_.v4.sin_family = AF_INET;
    addr_.v4.sin_port = 0;
    addr_.v4.sin_addr.s_addr = INADDR_ANY;
  }


  /**
   * Construct endpoint with specified \a protocol and \a port
   */
  constexpr basic_endpoint_t (const protocol_t &protocol, port_t port) noexcept
  {
    if (protocol.family() == AF_INET)
    {
      addr_.v4.sin_family = AF_INET;
      addr_.v4.sin_port = htons(port);
      addr_.v4.sin_addr.s_addr = INADDR_ANY;
      return;
    }

    addr_.v6.sin6_family = AF_INET6;
    addr_.v6.sin6_port = htons(port);
    addr_.v6.sin6_flowinfo = 0;
    addr_.v6.sin6_addr = IN6ADDR_ANY_INIT;
    addr_.v6.sin6_scope_id = 0;
  }


  /**
   * Construct endpoint with specified \a address and \a port
   */
  constexpr basic_endpoint_t (const address_t &address, port_t port) noexcept
  {
    if (const auto *addr = address.as_v4())
    {
      addr_.v4.sin_family = AF_INET;
      addr_.v4.sin_port = htons(port);
      addr->store(addr_.v4.sin_addr);
      return;
    }

    const auto *addr = address.as_v6();
    addr_.v6.sin6_family = AF_INET6;
    addr_.v6.sin6_port = htons(port);
    addr_.v6.sin6_flowinfo = 0;
    addr_.v6.sin6_scope_id = 0;
    addr->store(addr_.v6.sin6_addr);
  }


  /**
   * Return instance of endpoint's protocol.
   */
  constexpr protocol_t protocol () const noexcept
  {
    return protocol_t{addr_.data.ss_family};
  }


  /**
   * Return endpoint's address
   */
  constexpr address_t address () const noexcept
  {
    return address_t{addr_.data};
  }


  /**
   * Set new endpoint address. Port and other possible internal sockaddr data
   * fields are unchanged.
   */
  void address (const address_t &address) noexcept
  {
    if (const auto *addr = address.as_v4())
    {
      addr_.v4.sin_family = AF_INET;
      addr->store(addr_.v4.sin_addr);
      return;
    }

    const auto *addr = address.as_v6();
    addr_.v6.sin6_family = AF_INET6;
    addr->store(addr_.v6.sin6_addr);
  }


  /**
   * Return endpoint's port (in host byte order).
   */
  constexpr port_t port () const noexcept
  {
    if (addr_.data.ss_family == AF_INET)
    {
      return ntohs(addr_.v4.sin_port);
    }
    return ntohs(addr_.v6.sin6_port);
  }


  /**
   * Set endpoint's port (in host byte order).
   */
  void port (port_t port) noexcept
  {
    if (addr_.data.ss_family == AF_INET)
    {
      addr_.v4.sin_port = htons(port);
      return;
    }
    addr_.v6.sin6_port = htons(port);
  }


  /**
   * Return pointer to internal socket address data.
   */
  void *data () noexcept
  {
    return &addr_.data;
  }


  /**
   * Return pointer to internal socket address data.
   */
  const void *data () const noexcept
  {
    return &addr_.data;
  }


  /**
   * Return size of internal socket address data structure.
   */
  constexpr size_t size () const noexcept
  {
    if (addr_.data.ss_family == AF_INET)
    {
      return sizeof(addr_.v4);
    }
    return sizeof(addr_.v6);
  }


  /**
   * Set new size for internal socket address data structure. This is no-op
   * and throws \c std::length_error if \a s is different from family's
   * sockaddr size.
   */
  void resize (size_t s)
  {
    if (s != size())
    {
      throw_error<std::length_error>("basic_endpoint_t::resize");
    }
  }


  /**
   * Return family-independent sockaddr data structure size.
   */
  constexpr size_t capacity () const noexcept
  {
    return sizeof(addr_);
  }


  /**
   * Insert human readable \a endpoint representation into \a writer.
   */
  friend memory_writer_t &operator<< (memory_writer_t &writer,
    const basic_endpoint_t &endpoint) noexcept
  {
    if (endpoint.addr_.data.ss_family == AF_INET)
    {
      return writer.print(endpoint.address(), ':', endpoint.port());
    }
    return writer.print('[', endpoint.address(), "]:", endpoint.port());
  }


private:

  union
  {
    sockaddr_storage data;
    sockaddr_in v4;
    sockaddr_in6 v6;
  } addr_{};
};


/**
 * Create and insert human readable \a address into \a writer
 */
template <typename Protocol>
inline std::ostream &operator<< (std::ostream &os,
  const basic_endpoint_t<Protocol> &endpoint)
{
  char_array_t<INET6_ADDRSTRLEN + sizeof("[]:65535")> buf;
  buf << endpoint;
  return (os << buf.c_str());
}


}} // namespace net::ip


__sal_end
