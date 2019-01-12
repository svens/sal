#pragma once

/**
 * \file sal/uri/uri.hpp
 * URI library (RFC3986)
 *
 * \see https://tools.ietf.org/html/rfc3986
 */

#include <sal/config.hpp>
#include <sal/memory.hpp>
#include <sal/uri/error.hpp>
#include <sal/uri/scheme.hpp>
#include <sal/uri/view.hpp>


__sal_begin


namespace uri {


class uri_t
{
public:

  uri_t () = default;


  // normalized & decoded
  uri_t (std::string_view view, std::error_code &error) noexcept
    : view_(view, error)
  {
    if (!error)
    {
      init(error);
    }
  }


  // normalized & decoded
  uri_t (std::string_view view)
    : uri_t(view, throw_on_error("uri"))
  { }


  const view_t &view () const noexcept
  {
    return view_;
  }


  std::string encoded_string (std::error_code &error) const noexcept;


  std::string encoded_string () const
  {
    return encoded_string(throw_on_error("uri::encoded_string"));
  }


  // it is application responsibility to ensure \a name survives
  static void register_scheme (std::string_view name, const scheme_t &scheme);


private:

  view_t view_{};
  std::string uri_{};

  void init (std::error_code &error) noexcept;
};


template <typename It>
inline uri_t make_uri (It first, It last, std::error_code &error) noexcept
{
  return uri_t(to_view(first, last), error);
}


template <typename It>
inline uri_t make_uri (It first, It last)
{
  return uri_t(to_view(first, last));
}


template <typename Data>
inline uri_t make_uri (const Data &data, std::error_code &error) noexcept
{
  return uri_t(to_view(data), error);
}


template <typename Data>
inline uri_t make_uri (const Data &data)
{
  return uri_t(to_view(data));
}


} // namespace uri


__sal_end
