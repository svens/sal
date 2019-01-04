#pragma once

/**
 * \file sal/uri/error.hpp
 * URI library errors
 */

#include <sal/config.hpp>
#include <sal/error.hpp>


__sal_begin


namespace uri {


/**
 * URI handling error codes
 */
enum class errc
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
const std::error_category &category () noexcept;


/**
 * Make std::error_code from errc \a e
 */
inline std::error_code make_error_code (errc e) noexcept
{
  return std::error_code(static_cast<int>(e), category());
}


/**
 * Make std::error_condition from errc \a e
 */
inline std::error_condition make_error_condition (errc e) noexcept
{
  return std::error_condition(static_cast<int>(e), category());
}


} // namespace uri


__sal_end


namespace std {

template <>
struct is_error_condition_enum<sal::uri::errc>
  : public true_type
{};

} // namespace std
