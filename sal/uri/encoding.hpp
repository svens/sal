#pragma once

/**
 * \file sal/uri/encoding.hpp
 * URI encoding/decoding functions
 */

#include <sal/config.hpp>
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


} // namespace uri


__sal_end
