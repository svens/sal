#pragma once

/**
 * \file sal/net/ip/address_v4.hpp
 * Representation of IPv4 address.
 */


#include <sal/config.hpp>
#include <sal/net/__bits/platform.hpp>
#include <sal/char_array.hpp>
#include <array>
#include <cstdint>
#include <ostream>


__sal_begin


namespace net { namespace ip {


/**
 * IPv4 address.
 */
class address_v4_t
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
  constexpr bytes_t to_bytes () const noexcept
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
  bool is_unspecified () const noexcept
  {
    return to_uint() == 0;
  }


  /**
   * Return true if \a this is loopback address (127.0.0.0 - 127.255.255.255)
   */
  bool is_loopback () const noexcept
  {
    return (to_uint() & 0xff000000) == 0x7f000000;
  }


  /**
   * Return true if \a this is multicast address (224.0.0.0 - 239.255.255.255)
   */
  bool is_multicast () const noexcept
  {
    return (to_uint() & 0xf0000000) == 0xe0000000;
  }


  /**
   * Return true if \a this is private address (RFC1918)
   */
  bool is_private () const noexcept
  {
    auto as_uint = to_uint();
    return
      // 10.0.0.0 - 10.255.255.255
      (as_uint >= 0x0a000000 && as_uint <= 0x0affffff)
      // 172.16.0.0 - 172.31.255.255
      || (as_uint >= 0xac100000 && as_uint <= 0xac1fffff)
      // 192.168.0.0 - 192.168.255.255
      || (as_uint >= 0xc0a80000 && as_uint <= 0xc0a8ffff)
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
  static constexpr address_v4_t any () noexcept
  {
    return address_v4_t{INADDR_ANY};
  }


  /**
   * Return loopback address
   */
  static constexpr address_v4_t loopback () noexcept
  {
    return address_v4_t{INADDR_LOOPBACK};
  }


  /**
   * Return broadcast address
   */
  static constexpr address_v4_t broadcast () noexcept
  {
    return address_v4_t(INADDR_BROADCAST);
  }


  /**
   * Create and insert human readable \a address into \a writer
   */
  friend memory_writer_t &operator<< (memory_writer_t &writer,
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


private:

  union storage_t
  {
    in_addr in;
    bytes_t bytes;

    constexpr storage_t () noexcept
      : bytes{{0, 0, 0, 0}}
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
inline std::ostream &operator<< (std::ostream &os, const address_v4_t &a)
{
  char_array_t<sizeof("255.255.255.255")> buf;
  buf << a;
  return (os << buf.c_str());
}



/*
constexpr address_v4_t make_address_v4 (const address_v4_t::bytes_t &bytes)
{
}


constexpr address_v4_t make_address_v4 (const address_v4_t::uint_t &val)
{
}


constexpr address_v4_t make_address_v4 (v4_mapped_t, const address_v6_t &a);
{
}


constexpr address_v4_t make_address_v4 (const char *str)
{
}


constexpr address_v4_t make_address_v4 (const char *str, std::error_code &ec)
  noexcept
{
}


constexpr address_v4_t make_address_v4 (const std::string &str)
{
}


constexpr address_v4_t make_address_v4 (const std::string &str, std::error_code &ec)
  noexcept
{
}
*/


}} // namespace net::ip


__sal_end
