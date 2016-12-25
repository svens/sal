#pragma once

/**
 * \file sal/net/error.hpp
 */


#include <sal/config.hpp>
#include <sal/net/__bits/platform.hpp>
#include <sal/error.hpp>
#include <typeinfo>


__sal_begin


namespace net { namespace ip {


/**
 * Exception type thrown when invalid address casting between IPv4 and IPv6 is
 * attempted.
 */
class bad_address_cast_t
  : public std::bad_cast
{
};


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
 * returns pointer to string "sal::net::ip::resolver"
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


}} // namespace net::ip


__sal_end


namespace std {


template <>
struct is_error_condition_enum<sal::net::ip::resolver_errc_t>
  : public true_type
{};


inline std::error_code make_error_code (sal::net::ip::resolver_errc_t e)
{
  return std::error_code(static_cast<int>(e),
    sal::net::ip::resolver_category()
  );
}


} // namespace std
