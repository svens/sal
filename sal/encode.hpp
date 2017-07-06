#pragma once

/**
 * \file sal/encode.hpp
 * Various formats encode/decode
 */


#include <sal/config.hpp>
#include <sal/__bits/encode.hpp>
#include <memory>
#include <string>


__sal_begin


using hex_string = __bits::hex_string;
using base64 = __bits::base64;


template <typename Encoding, typename In>
inline size_t max_encoded_size (In first, In last) noexcept
{
  return Encoding::max_encoded_size(
    reinterpret_cast<const char *>(std::addressof(*first)),
    reinterpret_cast<const char *>(std::addressof(*last))
  );
}


template <typename Encoding, typename InPtr>
inline size_t max_encoded_size (const InPtr &data) noexcept
{
  return max_encoded_size<Encoding>(data.begin(), data.end());
}


template <typename Encoding, typename In, typename Out>
inline Out encode (In first, In last, Out d_first) noexcept
{
  return Encoding::encode(
    reinterpret_cast<const char *>(std::addressof(*first)),
    reinterpret_cast<const char *>(std::addressof(*last)),
    reinterpret_cast<char *>(std::addressof(*d_first))
  );
}


template <typename Encoding, typename InPtr, typename Out>
inline Out encode (const InPtr &data, Out d_first) noexcept
{
  return encode<Encoding>(data.begin(), data.end(), d_first);
}


template <typename Encoding, typename In>
inline std::string encode (In first, In last)
{
  auto result = std::string(max_encoded_size<Encoding>(first, last), '\0');
  auto end = encode<Encoding>(first, last, &result[0]);
  result.resize(end - &result[0]);
  return result;
}


template <typename Encoding, typename InPtr>
inline std::string encode (const InPtr &data)
{
  return encode<Encoding>(data.begin(), data.end());
}


__sal_end
