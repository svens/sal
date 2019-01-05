#pragma once

#include <sal/config.hpp>
#include <sal/uri/__bits/char_class.hpp>
#include <sal/uri/error.hpp>
#include <iterator>


__sal_begin


namespace uri::__bits {


template <typename Char>
constexpr Char decode_nibble (Char ch, std::error_code &error) noexcept
{
  if (is_digit(ch))
  {
    return ch - '0';
  }
  else if (in_range(ch, 'a', 'f'))
  {
    return ch - 'a' + 10;
  }
  else if (in_range(ch, 'A', 'F'))
  {
    return ch - 'A' + 10;
  }
  error = make_error_code(errc::invalid_hex_input);
  return ch;
}


template <typename InputIt, typename OutputIt>
constexpr OutputIt decode (InputIt first, InputIt last, OutputIt out,
  std::error_code &error)
{
  error.clear();
  for (/**/;  first != last && !error;  ++first, ++out)
  {
    if (*first != '%')
    {
      *out = *first;
    }
    else if (std::distance(first, last) > 2)
    {
      *out = decode_nibble(*++first, error) << 4 | decode_nibble(*++first, error);
    }
    else
    {
      error = make_error_code(errc::not_enough_input);
    }
  }
  return out;
}


template <typename Char>
constexpr Char encode_nibble (Char ch) noexcept
{
  return ch < 10 ? ch + '0' : ch + 'A' - 10;
}


template <typename InputIt, typename OutputIt, typename Filter>
constexpr OutputIt encode (InputIt first, InputIt last, OutputIt out, Filter safe)
{
  using char_type = std::decay_t<decltype(*first)>;

  while (first != last)
  {
    if (safe(*first))
    {
      *out++ = *first;
    }
    else
    {
      *out++ = '%';
      *out++ = static_cast<char_type>(encode_nibble((*first >> 4) & 0x0f));
      *out++ = static_cast<char_type>(encode_nibble(*first & 0x0f));
    }
    ++first;
  }
  return out;
}


} // namespace uri::__bits


__sal_end
