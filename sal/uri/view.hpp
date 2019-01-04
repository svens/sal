#pragma once

/**
 * \file sal/uri/view.hpp
 * Immutable URI view
 */

#include <sal/config.hpp>
#include <sal/uri/error.hpp>
#include <sal/memory.hpp>
#include <sal/net/fwd.hpp>
#include <string_view>


__sal_begin


namespace uri {


struct view_t
{
  std::string_view scheme{};
  std::string_view user_info{};
  std::string_view host{};
  std::string_view port{};
  net::ip::port_t port_value{};
  std::string_view path{};
  std::string_view query{};
  std::string_view fragment{};


  view_t () = default;


  view_t (const std::string_view &view, std::error_code &error) noexcept;


  view_t (const std::string_view &view)
    : view_t(view, throw_on_error("uri::view"))
  { }


  bool has_scheme () const noexcept
  {
    return !scheme.empty();
  }


  bool has_user_info () const noexcept
  {
    return user_info.data() != nullptr;
  }


  bool has_host () const noexcept
  {
    return !host.empty();
  }


  bool has_port () const noexcept
  {
    return !port.empty();
  }


  bool has_authority () const noexcept
  {
    return has_user_info() || has_host() || has_port();
  }


  bool has_path () const noexcept
  {
    return path.data() != nullptr;
  }


  bool has_query () const noexcept
  {
    return query.data() != nullptr;
  }


  bool has_fragment () const noexcept
  {
    return fragment.data() != nullptr;
  }
};


template <typename It>
inline view_t view (It first, It last, std::error_code &error) noexcept
{
  return view_t(as_view(first, last), error);
}


template <typename It>
inline view_t view (It first, It last)
{
  return view_t(as_view(first, last));
}


template <typename Data>
inline view_t view (const Data &data, std::error_code &error) noexcept
{
  return view_t(as_view(data), error);
}


template <typename Data>
inline view_t view (const Data &data)
{
  return view_t(as_view(data));
}


} // namespace uri


__sal_end
