#pragma once

/**
 * \file sal/encode.hpp
 * Various formats encode/decode
 */


#include <sal/config.hpp>
#include <sal/__bits/encode.hpp>
#include <sal/error.hpp>
#include <memory>
#include <string>
#include <vector>


__sal_begin


using hex_string = __bits::hex_string;
using base64 = __bits::base64;


/**
 * Encode
 */
template <typename Encoding, typename In>
inline size_t max_encoded_size (In first, In last) noexcept
{
  return Encoding::max_encoded_size(
    reinterpret_cast<const char *>(std::addressof(*first)),
    reinterpret_cast<const char *>(std::addressof(*last))
  );
}


/**
 * Encode
 */
template <typename Encoding, typename InPtr>
inline size_t max_encoded_size (const InPtr &data) noexcept
{
  return max_encoded_size<Encoding>(data.begin(), data.end());
}


/**
 * Encode
 */
template <typename Encoding, typename In, typename Out>
inline Out encode (In first, In last, Out d_first) noexcept
{
  return Encoding::encode(
    reinterpret_cast<const char *>(std::addressof(*first)),
    reinterpret_cast<const char *>(std::addressof(*last)),
    reinterpret_cast<char *>(std::addressof(*d_first))
  );
}


/**
 * Encode
 */
template <typename Encoding, typename InPtr, typename Out>
inline Out encode (const InPtr &data, Out d_first) noexcept
{
  return encode<Encoding>(data.begin(), data.end(), d_first);
}


/**
 * Encode
 */
template <typename Encoding, typename In>
inline std::string encode (In first, In last)
{
  auto result = std::string(max_encoded_size<Encoding>(first, last), '\0');
  auto end = encode<Encoding>(first, last, &result[0]);
  result.resize(end - &result[0]);
  return result;
}


/**
 * Encode
 */
template <typename Encoding, typename InPtr>
inline std::string encode (const InPtr &data)
{
  return encode<Encoding>(data.begin(), data.end());
}


/**
 * Decode
 */
template <typename Encoding, typename In>
inline size_t max_decoded_size (In first, In last, std::error_code &error)
  noexcept
{
  return Encoding::max_decoded_size(
    reinterpret_cast<const char *>(std::addressof(*first)),
    reinterpret_cast<const char *>(std::addressof(*last)),
    error
  );
}


/**
 * Decode
 */
template <typename Encoding, typename InPtr>
inline size_t max_decoded_size (const InPtr &data, std::error_code &error)
  noexcept
{
  return max_decoded_size<Encoding>(data.begin(), data.end(), error);
}


/**
 * Decode
 */
template <typename Encoding, typename In>
inline size_t max_decoded_size (In first, In last)
{
  return max_decoded_size<Encoding>(first, last,
    throw_on_error("max_decoded_size")
  );
}


/**
 * Decode
 */
template <typename Encoding, typename InPtr>
inline size_t max_decoded_size (const InPtr &data)
{
  return max_decoded_size<Encoding>(data, throw_on_error("max_decoded_size"));
}


/**
 * Decode
 */
template <typename Encoding, typename In, typename Out>
inline Out decode (In first, In last, Out d_first, std::error_code &error)
  noexcept
{
  return Encoding::decode(
    reinterpret_cast<const char *>(std::addressof(*first)),
    reinterpret_cast<const char *>(std::addressof(*last)),
    reinterpret_cast<char *>(std::addressof(*d_first)),
    error
  );
}


/**
 * Decode
 */
template <typename Encoding, typename In, typename Out>
inline Out decode (In first, In last, Out d_first)
{
  return decode<Encoding>(first, last, d_first, throw_on_error("decode"));
}


/**
 * Decode
 */
template <typename Encoding, typename InPtr, typename Out>
inline Out decode (const InPtr &data, Out d_first, std::error_code &error)
  noexcept
{
  return decode<Encoding>(data.begin(), data.end(), d_first, error);
}


/**
 * Decode
 */
template <typename Encoding, typename InPtr, typename Out>
inline Out decode (const InPtr &data, Out d_first)
{
  return decode<Encoding>(data, d_first, throw_on_error("decode"));
}


template <typename Encoding, typename In>
std::vector<char> decode (In first, In last, std::error_code &error) noexcept
{
  std::vector<char> result;
  auto size = max_decoded_size<Encoding>(first, last, error);
  if (!error)
  {
    result.resize(size);
    auto end = decode<Encoding>(first, last, &result[0], error);
    result.resize(end - &result[0]);
  }
  return result;
}


template <typename Encoding, typename In>
inline std::vector<char> decode (In first, In last)
{
  std::vector<char> result(max_decoded_size<Encoding>(first, last));
  auto end = decode<Encoding>(first, last, &result[0]);
  result.resize(end - &result[0]);
  return result;
}


template <typename Encoding, typename InPtr>
inline std::vector<char> decode (const InPtr &data, std::error_code &error)
  noexcept
{
  return decode<Encoding>(data.begin(), data.end(), error);
}


template <typename Encoding, typename InPtr>
inline std::vector<char> decode (const InPtr &data)
{
  return decode<Encoding>(data.begin(), data.end());
}


__sal_end
