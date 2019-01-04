#pragma once

#include <sal/config.hpp>
#include <array>
#include <initializer_list>


__sal_begin


namespace uri::__bits {


enum
{
  cc_space                  = 1 << 1,
  cc_digit                  = 1 << 2,
  cc_alpha                  = 1 << 3,
  cc_scheme                 = 1 << 4,
  cc_authority              = 1 << 5,
  cc_authority_separator    = 1 << 6,
  cc_user_info              = 1 << 7,
  cc_path                   = 1 << 8,
  cc_query                  = 1 << 9,
  cc_fragment               = 1 << 10,
};


constexpr bool allow_non_us_ascii (uint8_t v) noexcept
{
  return v >= 128;
}


constexpr bool in_list (uint8_t v, std::initializer_list<uint8_t> set) noexcept
{
  for (auto it: set)
  {
    if (it == v)
    {
      return true;
    }
  }
  return false;
}


constexpr bool in_range (uint8_t v, uint8_t l, uint8_t h) noexcept
{
  return l <= v && v <= h;
}


constexpr bool is_space (uint8_t ch) noexcept //{{{1
{
  return in_list(ch, {'\t', '\n', '\v', '\f', '\r', ' '});
}


constexpr bool is_digit (uint8_t ch) noexcept //{{{1
{
  return in_range(ch, '0', '9');
}


constexpr bool is_xdigit (uint8_t ch) noexcept //{{{1
{
  return is_digit(ch)
      || in_range(ch, 'a', 'f')
      || in_range(ch, 'A', 'F')
  ;
}


constexpr bool is_alpha (uint8_t ch) noexcept //{{{1
{
  return in_range(ch, 'a', 'z')
      || in_range(ch, 'A', 'Z')
  ;
}


constexpr bool is_alnum (uint8_t ch) noexcept //{{{1
{
  return is_alpha(ch)
      || is_digit(ch)
  ;
}


constexpr bool is_unreserved (uint8_t ch) noexcept //{{{1
{
  return is_alnum(ch)
      || in_list(ch, {'-', '.', '_', '~'})
  ;
}


constexpr bool is_pct_encoded (uint8_t ch) noexcept //{{{1
{
  return ch == '%'
      || is_xdigit(ch)
  ;
}


constexpr bool is_sub_delim (uint8_t ch) noexcept //{{{1
{
  return in_list(ch, {'!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='});
}


constexpr bool is_gen_delim (uint8_t ch) noexcept //{{{1
{
  return in_list(ch, {':', '/', '?', '#', '[', ']', '@'});
}


constexpr bool is_reserved (uint8_t ch) noexcept //{{{1
{
  return is_gen_delim(ch)
      || is_sub_delim(ch)
  ;
}


constexpr bool is_scheme (uint8_t ch) noexcept //{{{1
{
  return is_alnum(ch)
      || in_list(ch, {'+', '-', '.'})
    ;
}


constexpr bool is_user_info (uint8_t ch) noexcept //{{{1
{
  return is_unreserved(ch)
      || is_pct_encoded(ch)
      || is_sub_delim(ch)
      || ch == ':'
      || allow_non_us_ascii(ch)
  ;
}


constexpr bool is_host (uint8_t ch) noexcept //{{{1
{
  return is_unreserved(ch)
      || is_pct_encoded(ch)
      || is_sub_delim(ch)
      || in_list(ch, {'%', '[', ']'})
      || allow_non_us_ascii(ch)
  ;
}


constexpr bool is_port (uint8_t ch) noexcept //{{{1
{
  return is_digit(ch);
}


constexpr bool is_authority (uint8_t ch) noexcept //{{{1
{
  return is_user_info(ch)
      || ch == '@'
      || is_host(ch)
      || ch == ':'
      || is_port(ch)
  ;
}


constexpr bool is_authority_separator (uint8_t ch) noexcept //{{{1
{
  return in_list(ch, {'/', '?', '#'});
}


constexpr bool is_path (uint8_t ch) noexcept //{{{1
{
  return is_unreserved(ch)
      || is_sub_delim(ch)
      || in_list(ch, {'%', '/', ':', '@'})
      || allow_non_us_ascii(ch)
  ;
}


constexpr bool is_query (uint8_t ch) noexcept //{{{1
{
  return is_path(ch)
      || in_list(ch, {'/', '?'})
      || allow_non_us_ascii(ch)
  ;
}


constexpr bool is_fragment (uint8_t ch) noexcept //{{{1
{
  return is_path(ch)
      || in_list(ch, {'/', '?'})
      || allow_non_us_ascii(ch)
  ;
}

// }}}1


constexpr uint16_t classify (uint8_t ch) noexcept
{
  return (is_space(ch) ? cc_space : 0)
       | (is_digit(ch) ? cc_digit : 0)
       | (is_alpha(ch) ? cc_alpha : 0)
       | (is_scheme(ch) ? cc_scheme : 0)
       | (is_authority(ch) ? cc_authority : 0)
       | (is_authority_separator(ch) ? cc_authority_separator : 0)
       | (is_user_info(ch) ? cc_user_info : 0)
       | (is_path(ch) ? cc_path : 0)
       | (is_query(ch) ? cc_query : 0)
       | (is_fragment(ch) ? cc_fragment : 0)
  ;
}


template <size_t... Range>
constexpr auto generate_uri_cc (std::index_sequence<Range...>) noexcept
{
  return std::array<uint16_t, sizeof...(Range)>
  {
    {classify(Range)...},
  };
}


class uri_cc
{
public:

  static constexpr bool is_space (uint8_t ch) noexcept
  {
    return char_class_[ch] & cc_space;
  }

  static constexpr bool is_digit (uint8_t ch) noexcept
  {
    return char_class_[ch] & cc_digit;
  }

  static constexpr bool is_alpha (uint8_t ch) noexcept
  {
    return char_class_[ch] & cc_alpha;
  }

  static constexpr bool is_scheme (uint8_t ch) noexcept
  {
    return char_class_[ch] & cc_scheme;
  }

  static constexpr bool is_authority (uint8_t ch) noexcept
  {
    return char_class_[ch] & cc_authority;
  }

  static constexpr bool is_authority_separator (uint8_t ch) noexcept
  {
    return char_class_[ch] & cc_authority_separator;
  }

  static constexpr bool is_user_info (uint8_t ch) noexcept
  {
    return char_class_[ch] & cc_user_info;
  }

  static constexpr bool is_path (uint8_t ch) noexcept
  {
    return char_class_[ch] & cc_path;
  }

  static constexpr bool is_query (uint8_t ch) noexcept
  {
    return char_class_[ch] & cc_query;
  }

  static constexpr bool is_fragment (uint8_t ch) noexcept
  {
    return char_class_[ch] & cc_fragment;
  }


private:

  static constexpr std::array<uint16_t, 256> char_class_ =
    generate_uri_cc(std::make_index_sequence<256>());
};


} // namespace uri::__bits


__sal_end
