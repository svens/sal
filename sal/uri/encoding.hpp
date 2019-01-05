#pragma once

/**
 * \file sal/uri/encoding.hpp
 * URI encoding/decoding functions
 */

#include <sal/config.hpp>
#include <sal/uri/__bits/char_class.hpp>
#include <sal/uri/__bits/encoding.hpp>
#include <sal/error.hpp>
#include <iterator>


__sal_begin


namespace uri {


template <typename InputIt, typename OutputIt>
constexpr OutputIt decode (InputIt first, InputIt last, OutputIt out,
  std::error_code &error)
{
  return __bits::decode(first, last, out, error);
}


template <typename InputIt, typename OutputIt>
constexpr OutputIt decode (InputIt first, InputIt last, OutputIt out)
{
  return decode(first, last, out, throw_on_error("uri::decode"));
}


template <typename Data>
constexpr Data decode (const Data &input)
{
  Data result;
  using std::cbegin, std::cend;
  decode(cbegin(input), cend(input), std::back_inserter(result));
  return result;
}


template <typename InputIt, typename OutputIt, typename Filter>
constexpr OutputIt encode (InputIt first, InputIt last, OutputIt out,
  Filter filter)
{
  return __bits::encode(first, last, out, filter);
}


template <typename InputIt, typename OutputIt>
constexpr OutputIt encode_user_info (InputIt first, InputIt last, OutputIt out)
{
  return encode(first, last, out, __bits::uri_cc::is_user_info);
}


template <typename Data>
constexpr Data encode_user_info (const Data &input)
{
  Data result;
  using std::cbegin, std::cend;
  encode_user_info(cbegin(input), cend(input), std::back_inserter(result));
  return result;
}


template <typename InputIt, typename OutputIt>
constexpr OutputIt encode_path (InputIt first, InputIt last, OutputIt out)
{
  return encode(first, last, out, __bits::uri_cc::is_path);
}


template <typename Data>
constexpr Data encode_path (const Data &input)
{
  Data result;
  using std::cbegin, std::cend;
  encode_path(cbegin(input), cend(input), std::back_inserter(result));
  return result;
}


template <typename InputIt, typename OutputIt>
constexpr OutputIt encode_query (InputIt first, InputIt last, OutputIt out)
{
  return encode(first, last, out, __bits::uri_cc::is_query);
}


template <typename Data>
constexpr Data encode_query (const Data &input)
{
  Data result;
  using std::cbegin, std::cend;
  encode_query(cbegin(input), cend(input), std::back_inserter(result));
  return result;
}


template <typename InputIt, typename OutputIt>
constexpr OutputIt encode_fragment (InputIt first, InputIt last, OutputIt out)
{
  return encode(first, last, out, __bits::uri_cc::is_fragment);
}


template <typename Data>
constexpr Data encode_fragment (const Data &input)
{
  Data result;
  using std::cbegin, std::cend;
  encode_fragment(cbegin(input), cend(input), std::back_inserter(result));
  return result;
}


} // namespace uri


__sal_end
