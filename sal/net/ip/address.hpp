#pragma once

/**
 * \file sal/net/ip/address.hpp
 * IP addresses (v4, v6)
 */


#include <sal/config.hpp>
#include <sal/net/__bits/platform.hpp>
#include <sal/char_array.hpp>
#include <sal/error.hpp>
#include <array>
#include <cstdint>
#include <ostream>


__sal_begin


namespace net { namespace ip {


/**
 * IPv4 address.
 */
class address_v4_t // {{{1
{
public:

  /// Integer representation of IPv4 address
  using uint_t = uint_least32_t;

  /// Binary representation of IPv4 address
  using bytes_t = std::array<uint8_t, 4>;


  /**
   * Construct unspecified address (INADDR_ANY)
   */
  constexpr address_v4_t () noexcept = default;


  /**
   * Construct new address from \a bytes
   */
  constexpr address_v4_t (const bytes_t &bytes) noexcept
    : addr_{bytes}
  {}


  /**
   * Construct new address from integer \a val
   */
  explicit constexpr address_v4_t (uint_t val) noexcept
    : addr_{val}
  {}


  /**
   * Construct new address as copy of \a that
   */
  constexpr address_v4_t (const address_v4_t &that) noexcept
    : addr_{that.addr_.bytes}
  {}


  /**
   * Assign \a this fields from \a that
   */
  address_v4_t &operator= (const address_v4_t &that) noexcept
  {
    addr_.in.s_addr = that.addr_.in.s_addr;
    return *this;
  }


  /**
   * Return binary representation of \a this address
   */
  constexpr const bytes_t &to_bytes () const noexcept
  {
    return addr_.bytes;
  }


  /**
   * Return integer representation of \a this address
   */
  uint_t to_uint () const noexcept
  {
    return ntohl(addr_.in.s_addr);
  }


  /**
   * Return true if \a this is unspecified address (0.0.0.0)
   */
  constexpr bool is_unspecified () const noexcept
  {
    return addr_.in.s_addr == 0;
  }


  /**
   * Return true if \a this is loopback address (127.0.0.0 - 127.255.255.255)
   */
  constexpr bool is_loopback () const noexcept
  {
    return (addr_.bytes[0] & 0xff) == 0x7f;
  }


  /**
   * Return true if \a this is multicast address (224.0.0.0 - 239.255.255.255)
   */
  constexpr bool is_multicast () const noexcept
  {
    return (addr_.bytes[0] & 0xf0) == 0xe0;
  }


  /**
   * Return true if \a this is private address (RFC1918)
   */
  constexpr bool is_private () const noexcept
  {
    return
      // 10.0.0.0 - 10.255.255.255
      (addr_.bytes[0] == 0x0a)

      // 172.16.0.0 - 172.31.255.255
      || (addr_.bytes[0] == 0xac
        && (addr_.bytes[1] >= 0x10 && addr_.bytes[1] <= 0x1f))

      // 192.168.0.0 - 192.168.255.255
      || (addr_.bytes[0] == 0xc0 && addr_.bytes[1] == 0xa8)
    ;
  }


  /**
   * Return human readable textual representation of \a this address
   */
  std::string to_string () const
  {
    char_array_t<sizeof("255.255.255.255")> buf;
    buf << *this;
    return buf.to_string();
  }


  /**
   * Return unspecified address
   */
  static const address_v4_t &any () noexcept
  {
    static const address_v4_t addr_{INADDR_ANY};
    return addr_;
  }


  /**
   * Return loopback address
   */
  static const address_v4_t &loopback () noexcept
  {
    static const address_v4_t addr_{INADDR_LOOPBACK};
    return addr_;
  }


  /**
   * Return broadcast address
   */
  static const address_v4_t &broadcast () noexcept
  {
    static const address_v4_t addr_{INADDR_BROADCAST};
    return addr_;
  }


private:

  union storage_t
  {
    in_addr in;
    bytes_t bytes;

    constexpr storage_t () noexcept
      : bytes{}
    {}

    explicit constexpr storage_t (const bytes_t &bytes) noexcept
      : bytes{bytes}
    {}

    explicit constexpr storage_t (uint_t val) noexcept
      : bytes{{
          uint8_t((val >> 24) & 0xff),
          uint8_t((val >> 16) & 0xff),
          uint8_t((val >> 8) & 0xff),
          uint8_t((val >> 0) & 0xff),
        }}
    {}
  } addr_{};

  friend memory_writer_t &operator<< (memory_writer_t &writer,
    const address_v4_t &address
  ) noexcept;

  friend address_v4_t make_address_v4 (const char *str, std::error_code &ec)
    noexcept;
};


/// Return a.to_uint() == b.to_uint()
inline bool operator== (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return a.to_uint() == b.to_uint();
}


/// Return a.to_uint() != b.to_uint()
inline bool operator!= (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return !(a == b);
}


/// Return a.to_uint() < b.to_uint()
inline bool operator< (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return a.to_uint() < b.to_uint();
}


/// Return a.to_uint() > b.to_uint()
inline bool operator> (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return b < a;
}


/// Return a.to_uint() <= b.to_uint()
inline bool operator<= (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return !(b < a);
}


/// Return a.to_uint() >= b.to_uint()
inline bool operator>= (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return !(a < b);
}


/**
 * Create and insert human readable \a address into \a writer
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const address_v4_t &address) noexcept
{
  if (__bits::ntop(address.addr_.in, writer.begin(), writer.safe_size()))
  {
    writer.skip_until('\0');
  }
  else
  {
    writer.skip(INET_ADDRSTRLEN);
  }
  return writer;
}


/**
 * Create and insert human readable \a address into \a writer
 */
inline std::ostream &operator<< (std::ostream &os, const address_v4_t &a)
{
  char_array_t<sizeof("255.255.255.255")> buf;
  buf << a;
  return (os << buf.c_str());
}



/**
 * Create and return IPv4 address from \a bytes
 */
constexpr address_v4_t make_address_v4 (const address_v4_t::bytes_t &bytes)
{
  return address_v4_t{bytes};
}


/**
 * Create and return IPv4 address from \a val
 */
constexpr address_v4_t make_address_v4 (const address_v4_t::uint_t &val)
{
  return address_v4_t{val};
}


/**
 * Create and return IPv4 address from textual representation \a str. On
 * failure, set \a ec to \c std::errc::invalid_argument and return empty
 * address.
 */
inline address_v4_t make_address_v4 (const char *str, std::error_code &ec)
  noexcept
{
  address_v4_t address;
  if (__bits::pton(str, address.addr_.in))
  {
    return address;
  }
  ec = std::make_error_code(std::errc::invalid_argument);
  return address_v4_t{};
}


/**
 * Create and return IPv4 address from textual representation \a str. On
 * failure, throw std::system_error.
 */
inline address_v4_t make_address_v4 (const char *str)
{
  std::error_code ec;
  auto address = make_address_v4(str, ec);
  if (!ec)
  {
    return address;
  }
  throw_system_error(ec, "make_address_v4: ", str);
}


/**
 * Create and return IPv4 address from textual representation \a str. On
 * failure, set \a ec to \c std::errc::invalid_argument and return empty
 * address.
 */
inline address_v4_t make_address_v4 (const std::string &str, std::error_code &ec)
  noexcept
{
  return make_address_v4(str.c_str(), ec);
}


/**
 * Create and return IPv4 address from textual representation \a str. On
 * failure, throw std::system_error.
 */
inline address_v4_t make_address_v4 (const std::string &str)
{
  return make_address_v4(str.c_str());
}


/**
 * IPv6 address.
 *
 * Address type identification is defined by RFC4291
 * (https://tools.ietf.org/html/rfc4291)
 *
 * TODO http://pubs.opengroup.org/onlinepubs/000095399/basedefs/netinet/in.h.html
 */
class address_v6_t // {{{1
{
public:

  /// Binary representation of IPv6 address
  using bytes_t = std::array<uint8_t, 16>;

  /// Scope ID
  using scope_id_t = uint_least32_t;


  /**
   * Construct unspecified address
   */
  constexpr address_v6_t () noexcept = default;


  /**
   * Construct new address from \a bytes
   */
  constexpr address_v6_t (const bytes_t &bytes, scope_id_t scope=0) noexcept
    : addr_{bytes}
    , scope_{scope}
  {}


  /**
   * Construct new address as copy of \a that
   */
  constexpr address_v6_t (const address_v6_t &that) noexcept
    : addr_{that.addr_.bytes}
    , scope_{that.scope_}
  {}


  /**
   * Return binary representation of \a this address
   */
  constexpr const bytes_t &to_bytes () const noexcept
  {
    return addr_.bytes;
  }


  /**
   * Set scope id
   */
  void scope_id (scope_id_t id) noexcept
  {
    scope_ = id;
  }


  /**
   * Get scope id
   */
  constexpr scope_id_t scope_id () const noexcept
  {
    return scope_;
  }


  /**
   * Return true if \a this is unspecified address (::)
   */
  bool is_unspecified () const noexcept
  {
    return IN6_IS_ADDR_UNSPECIFIED(&addr_.in) != 0;
  }


  /**
   * Return true if \a this is loopback address (::1)
   */
  bool is_loopback () const noexcept
  {
    return IN6_IS_ADDR_LOOPBACK(&addr_.in) != 0;
  }


  /**
   * Return true if \a this is unicast link-local address
   */
  bool is_link_local () const noexcept
  {
    return IN6_IS_ADDR_LINKLOCAL(&addr_.in) != 0;
  }


  /**
   * Return true if \a this is unicast site-local address
   */
  bool is_site_local () const noexcept
  {
    return IN6_IS_ADDR_SITELOCAL(&addr_.in) != 0;
  }


  /**
   * Return true if \a this represents IPv4-mapped IPv6 address
   */
  bool is_v4_mapped () const noexcept
  {
    return IN6_IS_ADDR_V4MAPPED(&addr_.in) != 0;
  }


  /**
   * Return true if \a this is multicast address
   */
  bool is_multicast () const noexcept
  {
    return IN6_IS_ADDR_MULTICAST(&addr_.in) != 0;
  }


  /**
   * Return true if \a this is multicast node-local address
   */
  bool is_multicast_node_local () const noexcept
  {
    return IN6_IS_ADDR_MC_NODELOCAL(&addr_.in) != 0;
  }


  /**
   * Return true if \a this is multicast link-local address
   */
  bool is_multicast_link_local () const noexcept
  {
    return IN6_IS_ADDR_MC_LINKLOCAL(&addr_.in) != 0;
  }


  /**
   * Return true if \a this is multicast site-local address
   */
  bool is_multicast_site_local () const noexcept
  {
    return IN6_IS_ADDR_MC_SITELOCAL(&addr_.in) != 0;
  }


  /**
   * Return true if \a this is multicast organisation-local address
   */
  bool is_multicast_org_local () const noexcept
  {
    return IN6_IS_ADDR_MC_ORGLOCAL(&addr_.in) != 0;
  }


  /**
   * Return true if \a this is multicat global address
   */
  bool is_multicast_global () const noexcept
  {
    return IN6_IS_ADDR_MC_GLOBAL(&addr_.in) != 0;
  }


  /**
   * Return unspecified address
   */
  static const address_v6_t &any () noexcept
  {
    static const address_v6_t addr_{in6addr_any};
    return addr_;
  }


  /**
   * Return loopback address
   */
  static const address_v6_t &loopback () noexcept
  {
    static const address_v6_t addr_{in6addr_loopback};
    return addr_;
  }


private:

  union storage_t
  {
    in6_addr in;
    bytes_t bytes;

    constexpr storage_t () noexcept
      : bytes{}
    {}

    explicit constexpr storage_t (const bytes_t &bytes) noexcept
      : bytes{bytes}
    {}

    explicit constexpr storage_t (const in6_addr &addr) noexcept
      : in{addr}
    {}
  } addr_{};

  scope_id_t scope_{};


  constexpr address_v6_t (const in6_addr &addr) noexcept
    : addr_{addr}
  {}
};


}} // namespace net::ip


__sal_end
