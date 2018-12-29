#pragma once

/**
 * \file sal/uri.hpp
 * URI parser (RFC 3986)
 *
 * \see https://tools.ietf.org/html/rfc3986
 */

#include <sal/config.hpp>
#include <sal/error.hpp>
#include <sal/memory.hpp>
#include <string_view>


__sal_begin


struct uri_view_t
{
  uri_view_t () = default;

  uri_view_t (const std::string_view &view, std::error_code &error) noexcept;

  uri_view_t (const std::string_view &view)
    : uri_view_t(view, throw_on_error("uri_view"))
  { }

  std::string_view scheme{};
  std::string_view user_info{};
  std::string_view host{};
  std::string_view port{};
  std::string_view path{};
  std::string_view query{};
  std::string_view fragment{};
};


template <typename It>
inline uri_view_t uri_view (It first, It last, std::error_code &error) noexcept
{
  return uri_view_t(as_view(first, last), error);
}


template <typename It>
inline uri_view_t uri_view (It first, It last)
{
  return uri_view_t(as_view(first, last));
}


template <typename Data>
inline uri_view_t uri_view (const Data &data, std::error_code &error) noexcept
{
  return uri_view_t(as_view(data), error);
}


template <typename Data>
inline uri_view_t uri_view (const Data &data)
{
  return uri_view_t(as_view(data));
}


/**
 * URI handling error codes
 */
enum class uri_errc
{
  invalid_syntax = 1,
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
