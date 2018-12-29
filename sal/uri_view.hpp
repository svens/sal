#pragma once

/**
 * \file sal/uri_view.hpp
 * Immutable URI view
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


__sal_end
