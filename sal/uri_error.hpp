#pragma once

/**
 * \file sal/uri_error.hpp
 * URI library errors
 */

#include <sal/config.hpp>
#include <sal/error.hpp>


__sal_begin


/**
 * URI handling error codes
 */
enum class uri_errc
{
  invalid_scheme = 1,
  invalid_authority,
  invalid_port,
  invalid_path,
  invalid_query,
  invalid_fragment,
};


/**
 * Return reference to URI error category. The name virtual function returns
 * pointer to string "uri"
 */
const std::error_category &uri_category () noexcept;


/**
 * Make std::error_code from uri_errc \a e
 */
inline std::error_code make_error_code (uri_errc e) noexcept
{
  return std::error_code(static_cast<int>(e), uri_category());
}


/**
 * Make std::error_condition from uri_errc \a e
 */
inline std::error_condition make_error_condition (uri_errc e) noexcept
{
  return std::error_condition(static_cast<int>(e), uri_category());
}


__sal_end


namespace std {

template <>
struct is_error_condition_enum<sal::uri_errc>
  : public true_type
{};

} // namespace std
