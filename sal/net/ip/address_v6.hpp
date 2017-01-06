#pragma once

/**
 * \file sal/net/ip/address_v6.hpp
 * IPv6 address
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/error.hpp>
#include <sal/net/ip/address_v4.hpp>
#include <sal/char_array.hpp>
#include <array>
#include <cstdint>
#include <ostream>


__sal_begin


namespace net { namespace ip {


/**
 * IPv6 address.
 *
 * Address type identification is defined by RFC4291
 * (https://tools.ietf.org/html/rfc4291)
 */
class address_v6_t
{
public:

  /// Binary representation of IPv6 address
  using bytes_t = std::array<uint8_t, 16>;


  /**
   * Construct unspecified address
   */
  constexpr address_v6_t () noexcept = default;


  /**
   * Construct new address from \a bytes
   */
  constexpr address_v6_t (const bytes_t &bytes) noexcept
    : addr_{bytes}
  {}


  /**
   * Construct new address directly from in6_addr \a that
   */
  address_v6_t (const in6_addr &that) noexcept
  {
    load(that);
  }


  /**
   * Copy IPv6 address data from low-level in6_addr \a a
   */
  void load (const in6_addr &a) noexcept
  {
    std::memcpy(&addr_.in, &a, sizeof(addr_.in));
  }


  /**
   * Copy this IPv4 address data into low-level in6_addr \a a.
   */
  void store (in6_addr &a) const noexcept
  {
    std::memcpy(&a, &addr_.in, sizeof(a));
  }


  /**
   * Return binary representation of \a this address
   */
  constexpr const bytes_t &to_bytes () const noexcept
  {
    return addr_.bytes;
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
   * Return human readable textual representation of \a this address
   */
  std::string to_string () const
  {
    char_array_t<INET6_ADDRSTRLEN> buf;
    buf << *this;
    return buf.to_string();
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


  /**
   * Compare \a this to \a that. Return value has same meaning as std::memcmp
   */
  int compare (const address_v6_t &that) const noexcept
  {
    return std::memcmp(&addr_.in, &that.addr_.in, sizeof(addr_.in));
  }


  /**
   * Calculate hash value for \a this.
   */
  size_t hash () const noexcept
  {
    return __bits::combine(AF_INET6,
      __bits::fnv_1a(addr_.bytes.data(), addr_.bytes.data() + addr_.bytes.size())
    );
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

  friend memory_writer_t &operator<< (memory_writer_t &writer,
    const address_v6_t &address
  ) noexcept;

  friend address_v6_t make_address_v6 (const char *str, std::error_code &ec)
    noexcept;
};


/**
 * Return true if \a a == \a b
 */
inline bool operator== (const address_v6_t &a, const address_v6_t &b) noexcept
{
  return a.compare(b) == 0;
}


/**
 * Return true if \a a < \a b
 */
inline bool operator< (const address_v6_t &a, const address_v6_t &b) noexcept
{
  return a.compare(b) < 0;
}


/**
 * Return true if \a a != \a b
 */
inline bool operator!= (const address_v6_t &a, const address_v6_t &b) noexcept
{
  return a.compare(b) != 0;
}


/**
 * Return true if \a a > \a b
 */
inline bool operator> (const address_v6_t &a, const address_v6_t &b) noexcept
{
  return a.compare(b) > 0;
}


/**
 * Return true if \a a <= \a b
 */
inline bool operator<= (const address_v6_t &a, const address_v6_t &b) noexcept
{
  return a.compare(b) <= 0;
}


/**
 * Return true if \a a >= \a b
 */
inline bool operator>= (const address_v6_t &a, const address_v6_t &b) noexcept
{
  return a.compare(b) >= 0;
}


/**
 * Insert human readable \a address into \a writer
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const address_v6_t &address) noexcept
{
  if (__bits::ntop(address.addr_.in, writer.begin(), writer.safe_size()))
  {
    writer.skip_until('\0');
  }
  else
  {
    writer.skip(INET6_ADDRSTRLEN);
  }
  return writer;
}


/**
 * Insert human readable \a address into std::ostream \a os.
 */
inline std::ostream &operator<< (std::ostream &os, const address_v6_t &address)
{
  char_array_t<INET6_ADDRSTRLEN> buf;
  buf << address;
  return (os << buf.c_str());
}


/**
 * Create and return IPv6 address from \a bytes
 */
constexpr address_v6_t make_address_v6 (const address_v6_t::bytes_t &bytes)
  noexcept
{
  return address_v6_t{bytes};
}


/**
 * Create and return IPv6 address from textual representation \a str. On
 * failure, set \a ec to \c std::errc::invalid_argument and return empty
 * address.
 */
inline address_v6_t make_address_v6 (const char *str, std::error_code &ec)
  noexcept
{
  address_v6_t address;
  if (__bits::pton(str, address.addr_.in))
  {
    return address;
  }
  ec = std::make_error_code(std::errc::invalid_argument);
  return address_v6_t{};
}


/**
 * Create and return IPv6 address from textual representation \a str. On
 * failure, throw std::system_error.
 */
inline address_v6_t make_address_v6 (const char *str)
{
  return make_address_v6(str, throw_on_error("make_address_v6"));
}


/**
 * Create and return IPv4 address from textual representation \a str. On
 * failure, set \a ec to \c std::errc::invalid_argument and return empty
 * address.
 */
inline address_v6_t make_address_v6 (const std::string &str, std::error_code &ec)
  noexcept
{
  return make_address_v6(str.c_str(), ec);
}


/**
 * Create and return IPv4 address from textual representation \a str. On
 * failure, throw std::system_error.
 */
inline address_v6_t make_address_v6 (const std::string &str)
{
  return make_address_v6(str.c_str());
}


/**
 * Return address_v4_t object corresponding to the IPv4-mapped IPv6 address.
 * If \a a.is_v4_mapped() is false, return unspecified IPv4 address and set
 * \a ec to \c std::errc::invalid_argument.
 */
inline address_v4_t make_address_v4 (const address_v6_t &a, std::error_code &ec)
  noexcept
{
  if (a.is_v4_mapped())
  {
    const auto &v6b = a.to_bytes();
    address_v4_t::bytes_t bytes =
    {
      { v6b[12], v6b[13], v6b[14], v6b[15] }
    };
    return address_v4_t{bytes};
  }
  ec = std::make_error_code(std::errc::invalid_argument);
  return address_v4_t{};
}


/**
 * Return address_v4_t object corresponding to the IPv4-mapped IPv6 address.
 * If \a a.is_v4_mapped() is false, throw bad_address_cast_t.
 */
inline address_v4_t make_address_v4 (const address_v6_t &a)
{
  std::error_code ec;
  auto address = make_address_v4(a, ec);
  if (!ec)
  {
    return address;
  }
  __bits::bad_address_cast();
}


/**
 * Return address_v6_t object containing IPv4-mapped IPv6 address
 * corresponding to \a a.
 */
inline address_v6_t make_address_v6 (const address_v4_t &a)
  noexcept
{
  const auto &v4b = a.to_bytes();
  address_v6_t::bytes_t bytes =
  {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0xff, 0xff,
      v4b[0], v4b[1], v4b[2], v4b[3] }
  };
  return address_v6_t{bytes};
}


}} // namespace net::ip


__sal_end


namespace std {


template <>
struct hash<sal::net::ip::address_v6_t>
{
  size_t operator() (const sal::net::ip::address_v6_t &a) const noexcept
  {
    return a.hash();
  }
};


} // namespace std
