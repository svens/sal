#include <sal/uri_view.hpp>
#include <sal/uri_error.hpp>

#if __has_include(<charconv>)
  #include <charconv>
#else
  // TODO drop once charconv is provided
  #include <cstdlib>
  #include <limits>
#endif


__sal_begin


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


enum
{
  cc_space                = 1 << 1,
  cc_digit                = 1 << 2,
  cc_alpha                = 1 << 3,
  cc_scheme               = 1 << 4,
  cc_authority            = 1 << 5,
  cc_authority_separator  = 1 << 6,
  cc_user_info            = 1 << 7,
  cc_path                 = 1 << 8,
  cc_query                = 1 << 9,
  cc_fragment             = 1 << 10,
};


#if 0

#!python3

def classify(ch, classes):
    return [cl for cl in classes if globals()[cl](ch)]

def cc_space(ch): return ch.isspace()
def cc_digit(ch): return ch.isdigit()
def cc_alpha(ch): return ch.isalpha()
def cc_unreserved(ch): return ch.isalnum() or ch in ['-', '.', '_', '~']
def cc_gen_delim(ch): return ch in [':', '/', '?', '#', '[', ']', '@']
def cc_sub_delim(ch): return ch in ['!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '=']
def cc_scheme(ch): return ch.isalnum() or ch in ['+', '-', '.']
def cc_authority(ch): return cc_unreserved(ch) or cc_sub_delim(ch) or ch in ['%', '@', ':', '[', ']']
def cc_authority_separator(ch): return ch in ['/', '?', '#']
def cc_user_info(ch): return cc_unreserved(ch) or cc_sub_delim(ch) or ch in ['%', ':']
def cc_path(ch): return cc_unreserved(ch) or cc_sub_delim(ch) or ch in ['%', '/', ':', '@']
def cc_query(ch): return cc_path(ch) or ch in ['/', '?']
def cc_fragment(ch): return cc_path(ch) or ch in ['/', '?']

classes = [
    'cc_space',
    'cc_digit',
    'cc_alpha',
    'cc_scheme',
    'cc_authority',
    'cc_authority_separator',
    'cc_user_info',
    'cc_path',
    'cc_query',
    'cc_fragment',
]

for i in range(0, 128):
    ch = chr(i)
    cl = ' | '.join(classify(ch, classes))
    cl = cl if len(cl) > 0 else '0'
    ch = ch if ch.isprintable() else i
    print(f"/* {ch:<3} (0x{i:02x}) */ {cl},")

#endif


static constexpr uint16_t char_class[] = //{{{1
{
  /* 0   (0x00) */ 0,
  /* 1   (0x01) */ 0,
  /* 2   (0x02) */ 0,
  /* 3   (0x03) */ 0,
  /* 4   (0x04) */ 0,
  /* 5   (0x05) */ 0,
  /* 6   (0x06) */ 0,
  /* 7   (0x07) */ 0,
  /* 8   (0x08) */ 0,
  /* 9   (0x09) */ cc_space,
  /* 10  (0x0a) */ cc_space,
  /* 11  (0x0b) */ cc_space,
  /* 12  (0x0c) */ cc_space,
  /* 13  (0x0d) */ cc_space,
  /* 14  (0x0e) */ 0,
  /* 15  (0x0f) */ 0,
  /* 16  (0x10) */ 0,
  /* 17  (0x11) */ 0,
  /* 18  (0x12) */ 0,
  /* 19  (0x13) */ 0,
  /* 20  (0x14) */ 0,
  /* 21  (0x15) */ 0,
  /* 22  (0x16) */ 0,
  /* 23  (0x17) */ 0,
  /* 24  (0x18) */ 0,
  /* 25  (0x19) */ 0,
  /* 26  (0x1a) */ 0,
  /* 27  (0x1b) */ 0,
  /* 28  (0x1c) */ cc_space,
  /* 29  (0x1d) */ cc_space,
  /* 30  (0x1e) */ cc_space,
  /* 31  (0x1f) */ cc_space,
  /*     (0x20) */ cc_space,
  /* !   (0x21) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* "   (0x22) */ 0,
  /* #   (0x23) */ cc_authority_separator,
  /* $   (0x24) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* %   (0x25) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* &   (0x26) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* '   (0x27) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* (   (0x28) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* )   (0x29) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* *   (0x2a) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* +   (0x2b) */ cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* ,   (0x2c) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* -   (0x2d) */ cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* .   (0x2e) */ cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* /   (0x2f) */ cc_authority_separator | cc_path | cc_query | cc_fragment,
  /* 0   (0x30) */ cc_digit | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* 1   (0x31) */ cc_digit | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* 2   (0x32) */ cc_digit | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* 3   (0x33) */ cc_digit | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* 4   (0x34) */ cc_digit | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* 5   (0x35) */ cc_digit | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* 6   (0x36) */ cc_digit | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* 7   (0x37) */ cc_digit | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* 8   (0x38) */ cc_digit | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* 9   (0x39) */ cc_digit | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* :   (0x3a) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* ;   (0x3b) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* <   (0x3c) */ 0,
  /* =   (0x3d) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* >   (0x3e) */ 0,
  /* ?   (0x3f) */ cc_authority_separator | cc_query | cc_fragment,
  /* @   (0x40) */ cc_authority | cc_path | cc_query | cc_fragment,
  /* A   (0x41) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* B   (0x42) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* C   (0x43) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* D   (0x44) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* E   (0x45) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* F   (0x46) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* G   (0x47) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* H   (0x48) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* I   (0x49) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* J   (0x4a) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* K   (0x4b) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* L   (0x4c) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* M   (0x4d) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* N   (0x4e) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* O   (0x4f) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* P   (0x50) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* Q   (0x51) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* R   (0x52) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* S   (0x53) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* T   (0x54) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* U   (0x55) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* V   (0x56) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* W   (0x57) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* X   (0x58) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* Y   (0x59) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* Z   (0x5a) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* [   (0x5b) */ cc_authority,
  /* \   (0x5c) */ 0,
  /* ]   (0x5d) */ cc_authority,
  /* ^   (0x5e) */ 0,
  /* _   (0x5f) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* `   (0x60) */ 0,
  /* a   (0x61) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* b   (0x62) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* c   (0x63) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* d   (0x64) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* e   (0x65) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* f   (0x66) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* g   (0x67) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* h   (0x68) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* i   (0x69) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* j   (0x6a) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* k   (0x6b) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* l   (0x6c) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* m   (0x6d) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* n   (0x6e) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* o   (0x6f) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* p   (0x70) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* q   (0x71) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* r   (0x72) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* s   (0x73) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* t   (0x74) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* u   (0x75) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* v   (0x76) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* w   (0x77) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* x   (0x78) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* y   (0x79) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* z   (0x7a) */ cc_alpha | cc_scheme | cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* {   (0x7b) */ 0,
  /* |   (0x7c) */ 0,
  /* }   (0x7d) */ 0,
  /* ~   (0x7e) */ cc_authority | cc_user_info | cc_path | cc_query | cc_fragment,
  /* 127 (0x7f) */ 0,
}; //}}}1


constexpr bool is_scheme (int ch)
{
  return char_class[ch] & cc_scheme;
}


constexpr bool is_authority (int ch)
{
  return char_class[ch] & cc_authority;
}


constexpr bool is_authority_separator (int ch)
{
  return char_class[ch] & cc_authority_separator;
}


constexpr bool is_user_info (int ch)
{
  return char_class[ch] & cc_user_info;
}


constexpr bool is_path (int ch)
{
  return char_class[ch] & cc_path;
}


constexpr bool is_query (int ch)
{
  return char_class[ch] & cc_query;
}


constexpr bool is_fragment (int ch)
{
  return char_class[ch] & cc_fragment;
}


constexpr bool is_space (int ch)
{
  return char_class[ch] & cc_space;
}


constexpr bool is_space_or_zero (int ch)
{
  return is_space(ch) || ch == '\0';
}


constexpr bool is_digit (int ch)
{
  return char_class[ch] & cc_digit;
}


constexpr bool is_alpha (int ch)
{
  return char_class[ch] & cc_alpha;
}


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


uri_view_t::uri_view_t (const std::string_view &view, std::error_code &error)
  noexcept
{
  if (view.empty())
  {
    error.clear();
    return;
  }

  auto first = view.data();
  auto last = view.data() + view.length();

  first = skip_forward(first, last, is_space);
  last = skip_backward(first, last, is_space_or_zero);

  // check whether absolute URI or relative reference
  for (auto it = first;  it != last && *it != '/';  ++it)
  {
    if (*it == ':')
    {
      //
      // scheme
      //

      if (!is_alpha(*first))
      {
        error = make_error_code(uri_errc::invalid_scheme);
        return;
      }

      auto scheme_begin = first;
      first = skip_forward(scheme_begin, it, is_scheme);
      if (first < it)
      {
        error = make_error_code(uri_errc::invalid_scheme);
        return;
      }
      scheme = as_view(scheme_begin, first);

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
    if (first < last && !is_authority_separator(*first))
    {
      error = make_error_code(uri_errc::invalid_authority);
      return;
    }
    auto authority_end = first;

    if (authority_begin != authority_end)
    {
      //
      // host & port
      //

      auto port_begin = skip_backward(authority_begin, authority_end, is_digit);
      if (port_begin > authority_begin && port_begin[-1] == ':')
      {
        host = as_view(authority_begin, port_begin - 1);
        port = as_view(port_begin, authority_end);
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
            error = make_error_code(uri_errc::invalid_port);
            return;
          }
        }
      }
      else
      {
        host = as_view(authority_begin, authority_end);
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
        user_info = as_view(authority_begin, user_info_end);
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
      error = make_error_code(uri_errc::invalid_path);
      return;
    }
    path = as_view(path_begin, first);
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
      error = make_error_code(uri_errc::invalid_query);
      return;
    }
    query = as_view(query_begin, first);
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
      error = make_error_code(uri_errc::invalid_fragment);
      return;
    }
    fragment = as_view(fragment_begin, first);
  }

  error.clear();
}


__sal_end
