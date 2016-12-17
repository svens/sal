#pragma once

/**
 * \file sal/net/ip/address_v4.hpp
 * IPv4 address
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
    return addr_.in.s_addr == INADDR_ANY;
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


  /**
   * Compare \a this to \a that. Return value has same meaning as std::memcmp
   */
  int compare (const address_v4_t &that) const noexcept
  {
    auto this_ = to_uint(), that_ = that.to_uint();
    if (this_ > that_)
    {
      return 1;
    }
    else if (this_ < that_)
    {
      return -1;
    }
    return 0;
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


/**
 * Return true if \a a == \a b
 */
inline bool operator== (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return a.compare(b) == 0;
}


/**
 * Return true if \a a < \a b
 */
inline bool operator< (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return a.compare(b) < 0;
}


/**
 * Return true if \a a != \a b
 */
inline bool operator!= (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return a.compare(b) != 0;
}


/**
 * Return true if \a a > \a b
 */
inline bool operator> (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return a.compare(b) > 0;
}


/**
 * Return true if \a a <= \a b
 */
inline bool operator<= (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return a.compare(b) <= 0;
}


/**
 * Return true if \a a >= \a b
 */
inline bool operator>= (const address_v4_t &a, const address_v4_t &b) noexcept
{
  return a.compare(b) >= 0;
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
  char_array_t<INET_ADDRSTRLEN> buf;
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


}} // namespace net::ip


__sal_end
