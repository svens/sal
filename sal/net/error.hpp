#pragma once

/**
 * \file sal/net/error.hpp
 */


#include <sal/config.hpp>
#include <sal/net/ip/__bits/inet.hpp>
#include <sal/error.hpp>
#include <typeinfo>


__sal_begin


namespace net {

/**
 * Socket error codes
 */
enum class socket_errc_t
{
  already_open = 1,
  already_associated,
};


/**
 * Return reference to socket error category. The name virtual function
 * returns pointer to string "socket"
 */
const std::error_category &socket_category () noexcept;


/**
 * Make std::error_code from socket_errc_t \a e
 */
inline std::error_code make_error_code (socket_errc_t e) noexcept
{
  return std::error_code(static_cast<int>(e), socket_category());
}


/**
 * Make std::error_condition from socket_errc_t \a e
 */
inline std::error_condition make_error_condition (socket_errc_t e) noexcept
{
  return std::error_condition(static_cast<int>(e), socket_category());
}


/**
 * Helper class to catch std::error_code and throw std::system_error if there
 * was an error.
 *
 * \internal
 */
class throw_on_error
{
public:

  throw_on_error () = delete;
  throw_on_error (const throw_on_error &) = delete;
  throw_on_error &operator= (const throw_on_error &) = delete;

  /// Construct error thrower with message \a msg
  throw_on_error (const char *msg) noexcept
    : msg_(msg)
  {}

  ~throw_on_error () noexcept(false)
  {
    if (code_)
    {
      throw_system_error(code_, msg_);
    }
  }

  operator std::error_code& () noexcept
  {
    return code_;
  }

private:

  std::error_code code_{};
  const char * const msg_;
};


namespace ip {


/**
 * Exception type thrown when invalid address casting between IPv4 and IPv6 is
 * attempted.
 */
class bad_address_cast_t
  : public std::bad_cast
{};


namespace __bits {

inline void bad_address_cast [[noreturn]] ()
{
  throw bad_address_cast_t{};
}

} // namespace __bits


/**
 * Resolver error codes
 */
enum class resolver_errc_t
{
  host_not_found = EAI_NONAME,
  host_not_found_try_again = EAI_AGAIN,
  service_not_found = EAI_SERVICE,
};

/**
 * Return reference to resolver error category. The name virtual function
 * returns pointer to string "resolver"
 */
const std::error_category &resolver_category () noexcept;


/**
 * Make std::error_code from resolver_errc_t \a e
 */
inline std::error_code make_error_code (resolver_errc_t e) noexcept
{
  return std::error_code(static_cast<int>(e), resolver_category());
}


/**
 * Make std::error_condition from resolver_errc_t \a e
 */
inline std::error_condition make_error_condition (resolver_errc_t e) noexcept
{
  return std::error_condition(static_cast<int>(e), resolver_category());
}


} // namespace ip
} // namespace net


__sal_end


namespace std {

template <>
struct is_error_condition_enum<sal::net::socket_errc_t>
  : public true_type
{};

template <>
struct is_error_condition_enum<sal::net::ip::resolver_errc_t>
  : public true_type
{};

} // namespace std
