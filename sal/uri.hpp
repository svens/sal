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


struct uri_components_t
{
  std::string_view scheme{};
  std::string_view user_info{};
  std::string_view host{};
  std::string_view port{};
  std::string_view path{};
  std::string_view query{};
  std::string_view fragment{};
};


class uri_t
{
public:

  template <typename It>
  static uri_components_t parse (It first, It last, std::error_code &error)
    noexcept
  {
    if (first != last)
    {
      return parse(
        reinterpret_cast<const char *>(to_ptr(first)),
        reinterpret_cast<const char *>(to_end_ptr(first, last)),
        error
      );
    }
    return {};
  }


  template <typename It>
  static uri_components_t parse (It first, It last)
  {
    return parse(first, last, throw_on_error("uri::parse"));
  }


  template <typename Data>
  static uri_components_t parse (const Data &data, std::error_code &error)
    noexcept
  {
    using std::cbegin;
    using std::cend;
    return parse(cbegin(data), cend(data), error);
  }


  template <typename Data>
  static uri_components_t parse (const Data &data)
  {
    using std::cbegin;
    using std::cend;
    return parse(cbegin(data), cend(data));
  }


private:

  static uri_components_t parse (
    const char *first,
    const char *last,
    std::error_code &error
  ) noexcept;
};


/**
 * URI handling error codes
 */
enum class uri_errc
{
  invalid_scheme = 1,
  invalid_authority,
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
