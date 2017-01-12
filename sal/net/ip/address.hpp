#pragma once

/**
 * \file sal/net/ip/address.hpp
 * Version independent IP address
 */


#include <sal/config.hpp>
#include <sal/net/error.hpp>
#include <sal/net/__bits/socket.hpp>
#include <sal/net/ip/address_v4.hpp>
#include <sal/net/ip/address_v6.hpp>


__sal_begin


namespace net { namespace ip {


/**
 * Version independent representation of IP address. It holds either IPv4,
 * IPv6 or no valid address.
 */
class address_t
{
public:

  /**
   * Construct unspecified IPv4 address.
   */
  constexpr address_t () noexcept = default;


  /**
   * Construct new address as copy of \a that
   */
  constexpr address_t (const address_v4_t &that) noexcept
    : family_{AF_INET}
    , addr_{that}
  {}


  /**
   * Construct new address as copy of \a that
   */
  constexpr address_t (const address_v6_t &that) noexcept
    : family_{AF_INET6}
    , addr_{that}
  {}


  /**
   * Construct new address directly from sockaddr_storage \a that
   */
  address_t (const sockaddr_storage &that)
  {
    load(that);
  }


  /**
   * Try to copy IP address data from low-level sockaddr_storage. Return true
   * on success, and false if \a a family is not recognised.
   */
  bool try_load (const sockaddr_storage &a) noexcept
  {
    if (a.ss_family == AF_INET)
    {
      family_ = AF_INET;
      addr_.v4.load(reinterpret_cast<const sockaddr_in &>(a).sin_addr);
      return true;
    }
    else if (a.ss_family == AF_INET6)
    {
      family_ = AF_INET6;
      addr_.v6.load(reinterpret_cast<const sockaddr_in6 &>(a).sin6_addr);
      return true;
    }
    return false;
  }


  /**
   * Copy IP address data from low-level sockaddr_storage \a a. Throw
   * bad_address_cast_t if address family is not recognised.
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
    if (family_ == AF_INET)
    {
      addr_.v4.store(reinterpret_cast<sockaddr_in &>(a).sin_addr);
    }
    else
    {
      addr_.v6.store(reinterpret_cast<sockaddr_in6 &>(a).sin6_addr);
    }
    a.ss_family = family_;
  }


  /**
   * Return \c true if object contains IPv4 address, otherwise \c false
   */
  constexpr bool is_v4 () const noexcept
  {
    return family_ == AF_INET;
  }


  /**
   * Return \c true if object contains IPv6 address, otherwise \c false
   */
  constexpr bool is_v6 () const noexcept
  {
    return family_ == AF_INET6;
  }


  /**
   * Return IPv4 address. Throws bad_address_cast_t if is_v4() == false
   */
  const address_v4_t &to_v4 () const
  {
    if (auto a = as_v4())
    {
      return *a;
    }
    __bits::bad_address_cast();
  }


  /**
   * Return IPv6 address. Throws bad_address_cast_t if is_v6() == false
   */
  const address_v6_t &to_v6 () const
  {
    if (auto a = as_v6())
    {
      return *a;
    }
    __bits::bad_address_cast();
  }


  /**
   * Return pointer to internal IPv4 address or nullptr if is_v4() == false
   */
  constexpr const address_v4_t *as_v4 () const noexcept
  {
    return is_v4() ? &addr_.v4 : nullptr;
  }


  /**
   * Return pointer to internal IPv6 address or nullptr if is_v6() == false
   */
  constexpr const address_v6_t *as_v6 () const noexcept
  {
    return is_v6() ? &addr_.v6 : nullptr;
  }


  /**
   * Return true if \a this is unspecified address
   */
  constexpr bool is_unspecified () const noexcept
  {
    return is_v4() ? addr_.v4.is_unspecified() : addr_.v6.is_unspecified();
  }


  /**
   * Return true if \a this is loopback address
   */
  constexpr bool is_loopback () const noexcept
  {
    return is_v4() ? addr_.v4.is_loopback() : addr_.v6.is_loopback();
  }


  /**
   * Return true if \a this is multicast address
   */
  constexpr bool is_multicast () const noexcept
  {
    return is_v4() ? addr_.v4.is_multicast() : addr_.v6.is_multicast();
  }


  /**
   * Return human readable textual representation of \a this address
   */
  std::string to_string () const
  {
    return is_v4() ? addr_.v4.to_string() : addr_.v6.to_string();
  }


  /**
   * Compare \a this to \a that. Return value has same meaning as std::memcmp
   */
  int compare (const address_t &that) const noexcept
  {
    if (family_ == that.family_)
    {
      return is_v4()
        ? addr_.v4.compare(that.addr_.v4)
        : addr_.v6.compare(that.addr_.v6);
    }
    return family_ == AF_INET ? -1 : 1;
  }


  /**
   * Calculate hash value for \a this.
   */
  size_t hash () const noexcept
  {
    return is_v4() ? addr_.v4.hash() : addr_.v6.hash();
  }


private:

  net::__bits::sa_family_t family_{AF_INET};

  union storage_t
  {
    address_v4_t v4;
    address_v6_t v6;

    constexpr storage_t () noexcept
      : v4{}
    {}

    constexpr storage_t (const address_v4_t &that) noexcept
      : v4{that}
    {}

    constexpr storage_t (const address_v6_t &that) noexcept
      : v6{that}
    {}
  } addr_{};

  friend memory_writer_t &operator<< (memory_writer_t &writer,
    const address_t &address
  ) noexcept;
};


/**
 * Return true if \a a == \a b
 */
inline bool operator== (const address_t &a, const address_t &b) noexcept
{
  return a.compare(b) == 0;
}


/**
 * Return true if \a a < \a b
 */
inline bool operator< (const address_t &a, const address_t &b) noexcept
{
  return a.compare(b) < 0;
}


/**
 * Return true if \a a != \a b
 */
inline bool operator!= (const address_t &a, const address_t &b) noexcept
{
  return a.compare(b) != 0;
}


/**
 * Return true if \a a > \a b
 */
inline bool operator> (const address_t &a, const address_t &b) noexcept
{
  return a.compare(b) > 0;
}


/**
 * Return true if \a a <= \a b
 */
inline bool operator<= (const address_t &a, const address_t &b) noexcept
{
  return a.compare(b) <= 0;
}


/**
 * Return true if \a a >= \a b
 */
inline bool operator>= (const address_t &a, const address_t &b) noexcept
{
  return a.compare(b) >= 0;
}


/**
 * Insert human readable \a address into \a writer
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const address_t &address) noexcept
{
  if (address.family_ == AF_INET)
  {
    writer << address.addr_.v4;
  }
  else
  {
    writer << address.addr_.v6;
  }
  return writer;
}


/**
 * Insert human readable \a address into std::ostream \a os.
 */
inline std::ostream &operator<< (std::ostream &os, const address_t &address)
{
  char_array_t<INET6_ADDRSTRLEN> buf;
  buf << address;
  return (os << buf.c_str());
}


/**
 * Create and return address from textual representation \a str. On
 * failure, set \a ec to \c std::errc::invalid_argument and return unspecified
 * address.
 */
inline address_t make_address (const char *str, std::error_code &ec)
  noexcept
{
  auto a6 = make_address_v6(str, ec);
  if (!ec)
  {
    return a6;
  }
  ec.clear();
  return make_address_v4(str, ec);
}


/**
 * Create and return address from textual representation \a str. On failure,
 * throw std::system_error.
 */
inline address_t make_address (const char *str)
{
  return make_address(str, throw_on_error("make_address"));
}


/**
 * Create and return address from textual representation \a str. On failure,
 * set \a ec to \c std::errc::invalid_argument and return unspecified address.
 */
inline address_t make_address (const std::string &str, std::error_code &ec)
  noexcept
{
  return make_address(str.c_str(), ec);
}


/**
 * Create and return address from textual representation \a str. On failure,
 * throw std::system_error.
 */
inline address_t make_address (const std::string &str)
{
  return make_address(str.c_str());
}


}} // namespace net::ip


__sal_end


namespace std {


template <>
struct hash<sal::net::ip::address_t>
{
  size_t operator() (const sal::net::ip::address_t &a) const noexcept
  {
    return a.hash();
  }
};


} // namespace std
