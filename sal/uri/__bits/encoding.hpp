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
  for (/**/;  first != last;  ++first, ++out)
  {
    if (*first != '%')
    {
      *out = *first;
    }
    else if (std::distance(first, last) > 2)
    {
      *out = decode_nibble(first[1], error) << 4 | decode_nibble(first[2], error);
      if (!error)
      {
        first += 2;
      }
      else
      {
        break;
      }
    }
    else
    {
      error = make_error_code(errc::not_enough_input);
      break;
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

  for (/**/;  first != last;  ++first)
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
  }
  return out;
}


//
// special case for encoding query: any range is encoded as usual except
// associative containers that are encoded as list of key=value pairs
// separated by &
//


constexpr std::false_type is_associative_container (...) noexcept
{
  return {};
}


template <
  typename Data,
  typename = typename Data::key_type,
  typename = typename Data::mapped_type
>
constexpr std::true_type is_associative_container (const Data *) noexcept
{
  return {};
}


template <typename Data>
inline std::string encode_query_impl (const Data &input, std::false_type)
{
  std::string result;
  using std::cbegin, std::cend;
  encode(cbegin(input), cend(input),
    std::back_inserter(result),
    uri_cc::is_query
  );
  return result;
}


constexpr bool limited_query_charset (uint8_t ch) noexcept
{
  return ch != '=' && ch != '&' && uri_cc::is_query(ch);
}


template <typename Data>
inline std::string encode_query_impl (const Data &input, std::true_type)
{
  std::string result;
  auto out = std::back_inserter(result);
  bool prepend_separator = false;
  for (auto &[key, value]: input)
  {
    if (prepend_separator)
    {
      *out++ = '&';
    }
    else
    {
      prepend_separator = true;
    }
    encode(key.begin(), key.end(), out, limited_query_charset);
    *out++ = '=';
    encode(value.begin(), value.end(), out, limited_query_charset);
  }
  return result;
}


} // namespace uri::__bits


__sal_end
