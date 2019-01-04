#pragma once

#include <sal/config.hpp>
#include <sal/uri/__bits/char_class.hpp>
#include <sal/uri/error.hpp>
#include <iterator>


__sal_begin


namespace uri::__bits {


template <typename Char>
constexpr Char to_nibble (Char ch, std::error_code &error) noexcept
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
      *out = to_nibble(*++first, error) << 4 | to_nibble(*++first, error);
    }
    else
    {
      error = make_error_code(errc::not_enough_input);
    }
  }
  return out;
}


} // namespace uri::__bits


__sal_end
