#include <sal/uri/view.hpp>
#include <sal/uri/error.hpp>
#include <sal/uri/__bits/char_class.hpp>

#if __has_include(<charconv>)
  #include <charconv>
#else
  // TODO drop once charconv is provided
  #include <cstdlib>
  #include <limits>
#endif


__sal_begin


namespace uri {


namespace {

using __bits::uri_cc;
using __bits::is_extended_ascii;

template <typename Filter>
inline const char *skip_forward (const char *first, const char *last,
  Filter filter) noexcept
{
  while (first != last && filter(*first))
  {
    ++first;
  }
  return first;
}

template <typename Filter>
inline const char *skip_backward (const char *first, const char *last,
  Filter filter) noexcept
{
  while (last > first && filter(last[-1]))
  {
    --last;
  }
  return last;
}

constexpr bool is_authority (uint8_t ch) noexcept
{
  return uri_cc::is_authority(ch) || is_extended_ascii(ch);
}

constexpr bool is_user_info (uint8_t ch) noexcept
{
  return uri_cc::is_user_info(ch) || is_extended_ascii(ch);
}

constexpr bool is_path (uint8_t ch) noexcept
{
  return uri_cc::is_path(ch) || is_extended_ascii(ch);
}

constexpr bool is_query (uint8_t ch) noexcept
{
  return uri_cc::is_query(ch) || is_extended_ascii(ch);
}

constexpr bool is_fragment (uint8_t ch) noexcept
{
  return uri_cc::is_fragment(ch) || is_extended_ascii(ch);
}

} // namespace


view_t::view_t (const std::string_view &view, std::error_code &error) noexcept
{
  if (view.empty())
  {
    error.clear();
    return;
  }

  auto first = view.data();
  auto last = view.data() + view.length();

  // check whether absolute URI or relative reference
  for (auto it = first;  it != last && *it != '/';  ++it)
  {
    if (*it == ':')
    {
      //
      // scheme
      //

      if (!uri_cc::is_alpha(*first))
      {
        error = make_error_code(errc::invalid_scheme);
        return;
      }

      auto scheme_begin = first;
      first = skip_forward(scheme_begin, it, uri_cc::is_scheme);
      if (first < it)
      {
        error = make_error_code(errc::invalid_scheme);
        return;
      }
      scheme = to_view(scheme_begin, first);

      ++first;
      break;
    }
  }

  if (first + 1 < last && first[0] == '/' && first[1] == '/')
  {
    //
    // authority
    //

    auto authority_begin = first + 2;
    first = skip_forward(authority_begin, last, is_authority);
    if (first < last && !uri_cc::is_authority_separator(*first))
    {
      error = make_error_code(errc::invalid_authority);
      return;
    }
    auto authority_end = first;
    authority = to_view(authority_begin, authority_end);

    if (authority_begin != authority_end)
    {
      //
      // host & port
      //

      auto port_begin = skip_backward(
        authority_begin,
        authority_end,
        uri_cc::is_digit
      );
      if (port_begin > authority_begin && port_begin[-1] == ':')
      {
        host = to_view(authority_begin, port_begin - 1);
        port = to_view(port_begin, authority_end);
      }
      else
      {
        host = to_view(authority_begin, authority_end);
      }

      //
      // user info
      //

      auto user_info_end = skip_forward(
        host.data(),
        host.data() + host.length(),
        is_user_info
      );
      if (*user_info_end == '@')
      {
        user_info = to_view(authority_begin, user_info_end);
        host.remove_prefix(user_info_end - authority_begin + 1);
      }
    }
  }

  if (is_path(*first))
  {
    //
    // path
    //

    auto path_begin = first;
    first = skip_forward(path_begin, last, is_path);
    if (first < last && *first != '?' && *first != '#')
    {
      error = make_error_code(errc::invalid_path);
      return;
    }
    path = to_view(path_begin, first);
  }

  if (*first == '?')
  {
    //
    // query
    //

    auto query_begin = first + 1;
    first = skip_forward(query_begin, last, is_query);
    if (first < last && *first != '#')
    {
      error = make_error_code(errc::invalid_query);
      return;
    }
    query = to_view(query_begin, first);
  }

  if (*first == '#')
  {
    //
    // fragment
    //

    auto fragment_begin = first + 1;
    first = skip_forward(fragment_begin, last, is_fragment);
    if (first < last)
    {
      error = make_error_code(errc::invalid_fragment);
      return;
    }
    fragment = to_view(fragment_begin, first);
  }

  error.clear();
}


int view_t::compare (const view_t &that) const noexcept
{
  if (auto r = scheme.compare(that.scheme)) return r;
  if (auto r = user_info.compare(that.user_info)) return r;
  if (auto r = host.compare(that.host)) return r;
  if (auto r = port.compare(that.port)) return r;
  if (auto r = path.compare(that.path)) return r;
  if (auto r = query.compare(that.query)) return r;
  if (auto r = fragment.compare(that.fragment)) return r;
  return 0;
}


namespace {

#if __has_include(<charconv>)

  using std::from_chars;

#else

  struct from_chars_result {
    char *ptr{};
    std::errc ec{};
  };

  from_chars_result from_chars (
    const char *first,
    const char *last,
    net::ip::port_t &value,
    int base = 10) noexcept
  {
    from_chars_result result;
    auto v = std::strtoul(first, &result.ptr, base);
    if (result.ptr != last)
    {
      result.ec = std::errc::invalid_argument;
    }
    else if (v > std::numeric_limits<net::ip::port_t>::max())
    {
      result.ec = std::errc::result_out_of_range;
    }
    else
    {
      value = v;
    }
    return result;
  }

#endif

} // namespace


net::ip::port_t view_t::port_value (std::error_code &error) const noexcept
{
  net::ip::port_t result{};
  error = std::make_error_code(
    from_chars(port.data(), port.data() + port.length(), result).ec
  );
  return result;
}


namespace {

inline size_t estimated_length (const view_t &uri) noexcept
{
  return uri.scheme.length()
    + uri.user_info.length()
    + uri.host.length()
    + uri.port.length()
    + uri.path.length()
    + uri.query.length()
    + uri.fragment.length()
    + sizeof("://@:/?#")
  ;
}

} // namespace


std::string view_t::string () const
{
  std::string result;
  result.reserve(estimated_length(*this));

  if (has_scheme())
  {
    result += scheme;
    result += ':';
  }

  if (authority.data() != nullptr)
  {
    result += "//";

    if (has_user_info())
    {
      result += user_info;
      result += '@';
    }

    if (has_host())
    {
      result += host;
    }

    if (has_port())
    {
      result += ':';
      result += port;
    }
  }

  if (has_path())
  {
    result += path;
  }

  if (has_query())
  {
    result += '?';
    result += query;
  }

  if (has_fragment())
  {
    result += '#';
    result += fragment;
  }

  result.shrink_to_fit();
  return result;
}


} // namespace uri


__sal_end
