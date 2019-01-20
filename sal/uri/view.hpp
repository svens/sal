#pragma once

/**
 * \file sal/uri/view.hpp
 * Immutable URI view
 */

#include <sal/config.hpp>
#include <sal/uri/error.hpp>
#include <sal/hash.hpp>
#include <sal/memory.hpp>
#include <sal/net/fwd.hpp>
#include <ostream>
#include <string>
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


  void swap (view_t &that) noexcept
  {
    scheme.swap(that.scheme);
    authority.swap(that.authority);
    user_info.swap(that.user_info);
    host.swap(that.host);
    port.swap(that.port);
    path.swap(that.path);
    query.swap(that.query);
    fragment.swap(that.fragment);
  }


  bool empty () const noexcept
  {
    return !(has_scheme()
        || has_user_info()
        || has_host()
        || has_port()
        || has_path()
        || has_query()
        || has_fragment()
    );
  }


  bool has_scheme () const noexcept
  {
    return !scheme.empty();
  }


  bool has_authority () const noexcept
  {
    return has_user_info()
        || has_host()
        || has_port()
    ;
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


  int compare (const view_t &that) const noexcept;


  std::string string () const;
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


inline void swap (view_t &a, view_t &b) noexcept
{
  a.swap(b);
}


inline bool operator== (const view_t &a, const view_t &b) noexcept
{
  return a.compare(b) == 0;
}


inline bool operator!= (const view_t &a, const view_t &b) noexcept
{
  return a.compare(b) != 0;
}


inline bool operator< (const view_t &a, const view_t &b) noexcept
{
  return a.compare(b) < 0;
}


inline bool operator<= (const view_t &a, const view_t &b) noexcept
{
  return a.compare(b) <= 0;
}


inline bool operator> (const view_t &a, const view_t &b) noexcept
{
  return a.compare(b) > 0;
}


inline bool operator>= (const view_t &a, const view_t &b) noexcept
{
  return a.compare(b) >= 0;
}


inline std::ostream &operator<< (std::ostream &os, const view_t &view)
{
  return (os << view.string());
}


} // namespace uri


__sal_end


namespace std {

template <>
struct hash<sal::uri::view_t>
{
  size_t operator() (const sal::uri::view_t &view) const noexcept
  {
    hash<std::string_view> h;
    uint64_t result = h(view.scheme);
    result = sal::hash_128_to_64(h(view.user_info), result);
    result = sal::hash_128_to_64(h(view.host), result);
    result = sal::hash_128_to_64(h(view.port), result);
    result = sal::hash_128_to_64(h(view.path), result);
    result = sal::hash_128_to_64(h(view.query), result);
    result = sal::hash_128_to_64(h(view.fragment), result);
    return result;
  }
};

} // namespace std
