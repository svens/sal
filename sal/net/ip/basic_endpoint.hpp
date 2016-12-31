#pragma once

/**
 * \file sal/net/ip/basic_endpoint.hpp
 * IP endpoint (address/port pair)
 */


#include <sal/config.hpp>
#include <sal/net/error.hpp>
#include <sal/net/ip/address.hpp>
#include <sal/memory_writer.hpp>
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
  basic_endpoint_t () noexcept
  {
    addr_.v4.sin_family = AF_INET;
    addr_.v4.sin_port = htons(0);
    addr_.v4.sin_addr.s_addr = INADDR_ANY;
  }


  /**
   * Construct endpoint with specified \a protocol and \a port
   */
  basic_endpoint_t (const protocol_t &protocol, port_t port) noexcept
  {
    addr_.data.ss_family = static_cast<short>(protocol.family());
    if (addr_.data.ss_family == AF_INET)
    {
      addr_.v4.sin_port = htons(port);
      addr_.v4.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
      addr_.v6.sin6_port = htons(port);
      addr_.v6.sin6_flowinfo = 0;
      addr_.v6.sin6_addr = IN6ADDR_ANY_INIT;
      addr_.v6.sin6_scope_id = 0;
    }
  }


  /**
   * Construct endpoint with specified \a address and \a port
   */
  basic_endpoint_t (const address_t &address, port_t port) noexcept
  {
    address.store(addr_.data);
    if (addr_.data.ss_family == AF_INET)
    {
      addr_.v4.sin_port = htons(port);
    }
    else
    {
      addr_.v6.sin6_port = htons(port);
      addr_.v6.sin6_flowinfo = 0;
      addr_.v6.sin6_scope_id = 0;
    }
  }


  /**
   * Construct new address directly from sockaddr_storage \a that
   */
  basic_endpoint_t (const sockaddr_storage &that)
  {
    load(that);
  }


  /**
   * Try to copy IP address data from low-level sockaddr_storage \a a. On
   * success return true. Returns false if address family is not recognised.
   */
  bool try_load (const sockaddr_storage &a) noexcept
  {
    if (a.ss_family == AF_INET)
    {
      std::memcpy(&addr_.v4, &a, sizeof(addr_.v4));
      return true;
    }
    else if (a.ss_family == AF_INET6)
    {
      std::memcpy(&addr_.v6, &a, sizeof(addr_.v6));
      return true;
    }
    return false;
  }


  /**
   * Copy IP address data from low-level sockaddr_storage \a a
   */
  void load (const sockaddr_storage &a)
  {
    if (!try_load(a))
    {
      __bits::bad_address_cast();
    }
  }


  /**
   * Copy this IP address data into low-level sockaddr_storage \a a.
   */
  void store (sockaddr_storage &a) const noexcept
  {
    if (addr_.data.ss_family == AF_INET)
    {
      std::memcpy(&a, &addr_.v4, sizeof(addr_.v4));
    }
    else
    {
      std::memcpy(&a, &addr_.v6, sizeof(addr_.v6));
    }
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
    address.store(addr_.data);
  }


  /**
   * Return endpoint's port (in host byte order).
   */
  port_t port () const noexcept
  {
    return addr_.data.ss_family == AF_INET
      ? ntohs(addr_.v4.sin_port)
      : ntohs(addr_.v6.sin6_port)
    ;
  }


  /**
   * Set endpoint's port (in host byte order).
   */
  void port (port_t port) noexcept
  {
    if (addr_.data.ss_family == AF_INET)
    {
      addr_.v4.sin_port = htons(port);
    }
    else
    {
      addr_.v6.sin6_port = htons(port);
    }
  }


  /**
   * Return host name for address(). On error, return empty string
   */
  std::string host_name () const
  {
    char name[NI_MAXHOST] = { '\0' };
    ::getnameinfo(
      static_cast<const sockaddr *>(data()), static_cast<socklen_t>(size()),
      name, sizeof(name),
      nullptr, 0,
      (protocol().type() == SOCK_DGRAM ? NI_DGRAM : 0)
    );
    return name;
  }


  /**
   * Return service name for address(). On error, return empty string
   */
  std::string service_name () const
  {
    char name[NI_MAXSERV] = { '\0' };
    ::getnameinfo(
      static_cast<const sockaddr *>(data()), static_cast<socklen_t>(size()),
      nullptr, 0,
      name, sizeof(name),
      (protocol().type() == SOCK_DGRAM ? NI_DGRAM : 0)
    );
    return name;
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
    return addr_.data.ss_family == AF_INET
      ? sizeof(addr_.v4)
      : sizeof(addr_.v6)
    ;
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
   * Compare \a this to \a that. Return value has same meaning as std::memcmp
   */
  int compare (const basic_endpoint_t &that) const noexcept
  {
    auto r = addr_.data.ss_family - that.addr_.data.ss_family;
    if (r)
    {
      return r;
    }
    else if (addr_.data.ss_family == AF_INET)
    {
      r = addr_.v4.sin_addr.s_addr - that.addr_.v4.sin_addr.s_addr;
      return r ? r : addr_.v4.sin_port - that.addr_.v4.sin_port;
    }
    r = std::memcmp(&addr_.v6.sin6_addr,
      &that.addr_.v6.sin6_addr,
      sizeof(addr_.v6.sin6_addr)
    );
    return r ? r : addr_.v6.sin6_port - that.addr_.v6.sin6_port;
  }


  /**
   * Calculate hash value for \a this.
   */
  size_t hash () const noexcept
  {
    auto p = reinterpret_cast<const uint8_t *>(data());
    return __bits::fnv_1a(p, p + size());
  }


  /**
   * Insert human readable \a endpoint representation into \a writer.
   */
  friend memory_writer_t &operator<< (memory_writer_t &writer,
    const basic_endpoint_t &endpoint) noexcept
  {
    return endpoint.addr_.data.ss_family == AF_INET
      ? writer.print(endpoint.address(), ':', endpoint.port())
      : writer.print('[', endpoint.address(), "]:", endpoint.port())
    ;
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
 * Return true if \a a == \a b
 */
template <typename Protocol>
inline bool operator== (const basic_endpoint_t<Protocol> &a,
  const basic_endpoint_t<Protocol> &b) noexcept
{
  return a.compare(b) == 0;
}


/**
 * Return true if \a a < \a b
 */
template <typename Protocol>
inline bool operator< (const basic_endpoint_t<Protocol> &a,
  const basic_endpoint_t<Protocol> &b) noexcept
{
  return a.compare(b) < 0;
}


/**
 * Return true if \a a != \a b
 */
template <typename Protocol>
inline bool operator!= (const basic_endpoint_t<Protocol> &a,
  const basic_endpoint_t<Protocol> &b) noexcept
{
  return a.compare(b) != 0;
}


/**
 * Return true if \a a > \a b
 */
template <typename Protocol>
inline bool operator> (const basic_endpoint_t<Protocol> &a,
  const basic_endpoint_t<Protocol> &b) noexcept
{
  return a.compare(b) > 0;
}


/**
 * Return true if \a a <= \a b
 */
template <typename Protocol>
inline bool operator<= (const basic_endpoint_t<Protocol> &a,
  const basic_endpoint_t<Protocol> &b) noexcept
{
  return a.compare(b) <= 0;
}


/**
 * Return true if \a a >= \a b
 */
template <typename Protocol>
inline bool operator>= (const basic_endpoint_t<Protocol> &a,
  const basic_endpoint_t<Protocol> &b) noexcept
{
  return a.compare(b) >= 0;
}


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
