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
  std::string_view authority{};
  std::string_view user_info{};
  std::string_view host{};
  std::string_view port{};
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


  bool has_authority () const noexcept
  {
    return authority.data() != nullptr;
  }


  bool has_user_info () const noexcept
  {
    return user_info.data() != nullptr;
  }


  bool has_host () const noexcept
  {
    return host.data() != nullptr;
  }


  bool has_port () const noexcept
  {
    return port.data() != nullptr;
  }


  net::ip::port_t port_value (std::error_code &error) const noexcept;


  net::ip::port_t port_value () const
  {
    return port_value(throw_on_error("uri::view::port_value"));
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
inline view_t make_view (It first, It last, std::error_code &error) noexcept
{
  return {to_view(first, last), error};
}


template <typename It>
inline view_t make_view (It first, It last)
{
  return {to_view(first, last)};
}


template <typename Data>
inline view_t make_view (const Data &data, std::error_code &error) noexcept
{
  return {to_view(data), error};
}


template <typename Data>
inline view_t make_view (const Data &data)
{
  return {to_view(data)};
}


} // namespace uri


__sal_end
