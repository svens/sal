#pragma once

/**
 * \file sal/encode.hpp
 * Various formats encoding/decoding (hex, base64)
 */


#include <sal/config.hpp>
#include <sal/__bits/encode.hpp>
#include <sal/error.hpp>
#include <memory>
#include <string>
#include <vector>


__sal_begin


/**
 * Hex (base16) encoding
 */
using hex_string = __bits::hex_string;

/**
 * Base64 encoding (https://tools.ietf.org/html/rfc4648)
 */
using base64 = __bits::base64;


/**
 * Maximum size of output buffer (in bytes) required to encode data in range
 * [\a first, \a last).
 */
template <typename Encoding, typename In>
inline size_t max_encoded_size (In first, In last) noexcept
{
  if (first == last)
  {
    return 0;
  }

  auto begin = reinterpret_cast<const uint8_t *>(std::addressof(*first));
  auto end = begin + (last - first) * sizeof(*first);
  return Encoding::max_encoded_size(begin, end);
}


/**
 * Maximum size of output buffer (in bytes) required to encode \a data
 */
template <typename Encoding, typename InPtr>
inline size_t max_encoded_size (const InPtr &data) noexcept
{
  return max_encoded_size<Encoding>(data.begin(), data.end());
}


/**
 * Encode data in range [\a first, \a last) using \a Encoding. Result is
 * stored into range [\a d_first, \a d_first + max_encoded_size)
 *
 * \returns Iterator to one past final byte stored into output buffer.
 */
template <typename Encoding, typename In, typename Out>
inline Out encode (In first, In last, Out d_first) noexcept
{
  if (first == last)
  {
    return d_first;
  }

  auto begin = reinterpret_cast<const uint8_t *>(std::addressof(*first));
  auto end = begin + (last - first) * sizeof(*first);
  return reinterpret_cast<Out>(
    Encoding::encode(begin, end,
      reinterpret_cast<uint8_t *>(std::addressof(*d_first))
    )
  );
}


/**
 * Encode data in \a data using \a Encoding. Result is stored into range [\a
 * d_first, \a d_first + max_encoded_size)
 *
 * \returns Iterator to one past final byte stored into output buffer.
 */
template <typename Encoding, typename InPtr, typename Out>
inline Out encode (const InPtr &data, Out d_first) noexcept
{
  return encode<Encoding>(data.begin(), data.end(), d_first);
}


/**
 * Conveniency wrapper for encoding, returning output data wrapped into
 * std::string.
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
 * Conveniency wrapper for encoding, returning output data wrapped into
 * std::string.
 */
template <typename Encoding, typename InPtr>
inline std::string encode (const InPtr &data)
{
  return encode<Encoding>(data.begin(), data.end());
}


/**
 * Maximum size of output buffer (in bytes) required to decode data in range
 * [\a first, \a last). If input range has invalid number of bytes, \a error
 * is set to \c std::errc::message_size and 0 is returned
 */
template <typename Encoding, typename In>
inline size_t max_decoded_size (In first, In last, std::error_code &error)
  noexcept
{
  if (first == last)
  {
    return 0;
  }

  auto begin = reinterpret_cast<const uint8_t *>(std::addressof(*first));
  auto end = begin + (last - first) * sizeof(*first);
  return Encoding::max_decoded_size(begin, end, error);
}


/**
 * Maximum size of output buffer (in bytes) required to decode \a data. If
 * input range has invalid number of bytes, \a error is set to
 * \c std::errc::message_size and 0 is returned
 */
template <typename Encoding, typename InPtr>
inline size_t max_decoded_size (const InPtr &data, std::error_code &error)
  noexcept
{
  return max_decoded_size<Encoding>(data.begin(), data.end(), error);
}


/**
 * Maximum size of output buffer (in bytes) required to decode data in range
 * [\a first, \a last). If input range has invalid number of bytes, throws an
 * exception \c std::system_error.
 */
template <typename Encoding, typename In>
inline size_t max_decoded_size (In first, In last)
{
  return max_decoded_size<Encoding>(first, last,
    throw_on_error("max_decoded_size")
  );
}


/**
 * Maximum size of output buffer (in bytes) required to decode \a data. If
 * input range has invalid number of bytes, throws an exception
 * \c std::system_error.
 */
template <typename Encoding, typename InPtr>
inline size_t max_decoded_size (const InPtr &data)
{
  return max_decoded_size<Encoding>(data, throw_on_error("max_decoded_size"));
}


/**
 * Decode data in range [\a first, \a last), storing output into buffer
 * starting with \a d_first. Output buffer should have room for at least
 * max_decoded_size() bytes.
 *
 * \returns Iterator to one past final byte stored in output buffer. On
 * decoding failure, sets \a error to std::errc::illegal_byte_sequence and
 * returned iterator points one past last successfully decoded byte.
 */
template <typename Encoding, typename In, typename Out>
inline Out decode (In first, In last, Out d_first, std::error_code &error)
  noexcept
{
  if (first == last)
  {
    return d_first;
  }

  auto begin = reinterpret_cast<const uint8_t *>(std::addressof(*first));
  auto end = begin + (last - first) * sizeof(*first);
  return reinterpret_cast<Out>(
    Encoding::decode(begin, end,
      reinterpret_cast<uint8_t *>(std::addressof(*d_first)),
      error
    )
  );
}


/**
 * Decode data in range [\a first, \a last), storing output into buffer
 * starting with \a d_first. Output buffer should have room for at least
 * max_decoded_size() bytes.
 *
 * \returns Iterator to one past final byte stored in output buffer. On
 * decoding failure, throws std::system_error
 */
template <typename Encoding, typename In, typename Out>
inline Out decode (In first, In last, Out d_first)
{
  return decode<Encoding>(first, last, d_first, throw_on_error("decode"));
}


/**
 * Decode \a data, storing output into buffer starting with \a d_first. Output
 * buffer should have room for at least max_decoded_size() bytes.
 *
 * \returns Iterator to one past final byte stored in output buffer. On
 * decoding failure, sets \a error to std::errc::illegal_byte_sequence and
 * returned iterator points one past last successfully decoded byte.
 */
template <typename Encoding, typename InPtr, typename Out>
inline Out decode (const InPtr &data, Out d_first, std::error_code &error)
  noexcept
{
  return decode<Encoding>(data.begin(), data.end(), d_first, error);
}


/**
 * Decode \a data, storing output into buffer starting with \a d_first. Output
 * buffer should have room for at least max_decoded_size() bytes.
 *
 * \returns Iterator to one past final byte stored in output buffer. On
 * decoding failure, throws std::system_error
 */
template <typename Encoding, typename InPtr, typename Out>
inline Out decode (const InPtr &data, Out d_first)
{
  return decode<Encoding>(data, d_first, throw_on_error("decode"));
}


/**
 * Conveniency wrapper for decoding, returning output data wrapped into
 * std::vector<uint8_t>.
 */
template <typename Encoding, typename In>
inline std::vector<uint8_t> decode (In first, In last, std::error_code &error)
  noexcept
{
  std::vector<uint8_t> result;
  auto size = max_decoded_size<Encoding>(first, last, error);
  if (!error && size)
  {
    result.resize(size);
    auto end = decode<Encoding>(first, last, &result[0], error);
    result.resize(end - &result[0]);
  }
  return result;
}


/**
 * Conveniency wrapper for decoding, returning output data wrapped into
 * std::vector<uint8_t>.
 */
template <typename Encoding, typename In>
inline std::vector<uint8_t> decode (In first, In last)
{
  std::vector<uint8_t> result;
  if (auto size = max_decoded_size<Encoding>(first, last))
  {
    result.resize(size);
    auto end = decode<Encoding>(first, last, &result[0]);
    result.resize(end - &result[0]);
  }
  return result;
}


/**
 * Conveniency wrapper for decoding, returning output data wrapped into
 * std::vector<uint8_t>.
 */
template <typename Encoding, typename InPtr>
inline std::vector<uint8_t> decode (const InPtr &data, std::error_code &error)
  noexcept
{
  return decode<Encoding>(data.begin(), data.end(), error);
}


/**
 * Conveniency wrapper for decoding, returning output data wrapped into
 * std::vector<uint8_t>.
 */
template <typename Encoding, typename InPtr>
inline std::vector<uint8_t> decode (const InPtr &data)
{
  return decode<Encoding>(data.begin(), data.end());
}


__sal_end
