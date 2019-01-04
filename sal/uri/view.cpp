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

  first = skip_forward(first, last, uri_cc::is_space);
  last = skip_backward(first, last, [](auto ch)
  {
    return uri_cc::is_space(ch) || ch == '\0';
  });

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
    first = skip_forward(authority_begin, last, uri_cc::is_authority);
    if (first < last && !uri_cc::is_authority_separator(*first))
    {
      error = make_error_code(errc::invalid_authority);
      return;
    }
    auto authority_end = first;

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
        if (!port.empty())
        {
          auto e = from_chars(
            port.data(),
            port.data() + port.length(),
            port_value,
            10
          ).ec;
          if (e != std::errc{})
          {
            error = make_error_code(errc::invalid_port);
            return;
          }
        }
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
        uri_cc::is_user_info
      );
      if (*user_info_end == '@')
      {
        user_info = to_view(authority_begin, user_info_end);
        host.remove_prefix(user_info_end - authority_begin + 1);
      }
    }
  }

  if (uri_cc::is_path(*first))
  {
    //
    // path
    //

    auto path_begin = first;
    first = skip_forward(path_begin, last, uri_cc::is_path);
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
    first = skip_forward(query_begin, last, uri_cc::is_query);
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
    first = skip_forward(fragment_begin, last, uri_cc::is_fragment);
    if (first < last)
    {
      error = make_error_code(errc::invalid_fragment);
      return;
    }
    fragment = to_view(fragment_begin, first);
  }

  error.clear();
}


} // namespace uri


__sal_end
