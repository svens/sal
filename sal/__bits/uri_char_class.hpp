#pragma once

#include <sal/config.hpp>
#include <array>
#include <initializer_list>


__sal_begin


namespace __bits {

namespace {

enum
{
  uri_cc_space                  = 1 << 1,
  uri_cc_digit                  = 1 << 2,
  uri_cc_alpha                  = 1 << 3,
  uri_cc_scheme                 = 1 << 4,
  uri_cc_authority              = 1 << 5,
  uri_cc_authority_separator    = 1 << 6,
  uri_cc_user_info              = 1 << 7,
  uri_cc_path                   = 1 << 8,
  uri_cc_query                  = 1 << 9,
  uri_cc_fragment               = 1 << 10,
};

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

constexpr bool uri_char_class_space (uint8_t ch) noexcept //{{{1
{
  return in_list(ch, {'\t', '\n', '\v', '\f', '\r', ' '});
}

constexpr bool uri_char_class_digit (uint8_t ch) noexcept //{{{1
{
  return in_range(ch, '0', '9');
}

constexpr bool uri_char_class_alpha (uint8_t ch) noexcept //{{{1
{
  return in_range(ch, 'a', 'z') || in_range(ch, 'A', 'Z');
}

constexpr bool uri_char_class_alnum (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_alpha(ch) || uri_char_class_digit(ch);
}

constexpr bool uri_char_class_unreserved (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_alnum(ch) || in_list(ch, {'-', '.', '_', '~'});
}

constexpr bool uri_char_class_pct_encoded (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_digit(ch)
    || in_range(ch, 'a', 'f')
    || in_range(ch, 'A', 'F');
}

constexpr bool uri_char_class_sub_delim (uint8_t ch) noexcept //{{{1
{
  return in_list(ch, {'!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='});
}

constexpr bool uri_char_class_gen_delim (uint8_t ch) noexcept //{{{1
{
  return in_list(ch, {':', '/', '?', '#', '[', ']', '@'});
}

constexpr bool uri_char_class_reserved (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_gen_delim(ch) || uri_char_class_sub_delim(ch);
}

constexpr bool uri_char_class_scheme (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_alnum(ch) || in_list(ch, {'+', '-', '.'});
}

constexpr bool uri_char_class_user_info (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_unreserved(ch)
    || uri_char_class_pct_encoded(ch)
    || uri_char_class_sub_delim(ch)
    || ch == ':'
  ;
}

constexpr bool uri_char_class_host (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_unreserved(ch)
    || uri_char_class_pct_encoded(ch)
    || uri_char_class_sub_delim(ch)
    || in_list(ch, {'%', '[', ']'})
  ;
}

constexpr bool uri_char_class_port (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_digit(ch);
}

constexpr bool uri_char_class_authority (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_user_info(ch)
    || ch == '@'
    || uri_char_class_host(ch)
    || ch == ':'
    || uri_char_class_port(ch)
  ;
}

constexpr bool uri_char_class_authority_separator (uint8_t ch) noexcept //{{{1
{
  return in_list(ch, {'/', '?', '#'});
}

constexpr bool uri_char_class_path (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_unreserved(ch)
    || uri_char_class_sub_delim(ch)
    || in_list(ch, {'%', '/', ':', '@'})
  ;
}

constexpr bool uri_char_class_query (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_path(ch) || in_list(ch, {'/', '?'});
}

constexpr bool uri_char_class_fragment (uint8_t ch) noexcept //{{{1
{
  return uri_char_class_path(ch) || in_list(ch, {'/', '?'});
}

// }}}1

constexpr uint16_t uri_char_class (uint8_t ch) noexcept
{
  return (uri_char_class_space(ch) ? uri_cc_space : 0)
    | (uri_char_class_digit(ch) ? uri_cc_digit : 0)
    | (uri_char_class_alpha(ch) ? uri_cc_alpha : 0)
    | (uri_char_class_scheme(ch) ? uri_cc_scheme : 0)
    | (uri_char_class_authority(ch) ? uri_cc_authority : 0)
    | (uri_char_class_authority_separator(ch) ? uri_cc_authority_separator : 0)
    | (uri_char_class_user_info(ch) ? uri_cc_user_info : 0)
    | (uri_char_class_path(ch) ? uri_cc_path : 0)
    | (uri_char_class_query(ch) ? uri_cc_query : 0)
    | (uri_char_class_fragment(ch) ? uri_cc_fragment : 0)
  ;
}

template <size_t... Charset>
constexpr std::array<uint16_t, 256> generate_uri_char_class (
  std::index_sequence<Charset...>) noexcept
{
  return
  {
    {uri_char_class(Charset)...},
  };
}

} // namespace


class uri_cc
{
public:

  static constexpr bool is_space (uint8_t ch) noexcept
  {
    return char_class_[ch] & uri_cc_space;
  }

  static constexpr bool is_digit (uint8_t ch) noexcept
  {
    return char_class_[ch] & uri_cc_digit;
  }

  static constexpr bool is_alpha (uint8_t ch) noexcept
  {
    return char_class_[ch] & uri_cc_alpha;
  }

  static constexpr bool is_scheme (uint8_t ch) noexcept
  {
    return char_class_[ch] & uri_cc_scheme;
  }

  static constexpr bool is_authority (uint8_t ch) noexcept
  {
    return char_class_[ch] & uri_cc_authority;
  }

  static constexpr bool is_authority_separator (uint8_t ch) noexcept
  {
    return char_class_[ch] & uri_cc_authority_separator;
  }

  static constexpr bool is_user_info (uint8_t ch) noexcept
  {
    return char_class_[ch] & uri_cc_user_info;
  }

  static constexpr bool is_path (uint8_t ch) noexcept
  {
    return char_class_[ch] & uri_cc_path;
  }

  static constexpr bool is_query (uint8_t ch) noexcept
  {
    return char_class_[ch] & uri_cc_query;
  }

  static constexpr bool is_fragment (uint8_t ch) noexcept
  {
    return char_class_[ch] & uri_cc_fragment;
  }


private:

  static constexpr std::array<uint16_t, 256> char_class_ =
    generate_uri_char_class(std::make_index_sequence<256>());
};


} // namespace __bits


__sal_end
