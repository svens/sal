#pragma once

/**
 * \file sal/encode.hpp
 * Various formats encoding/decoding (hex, base64)
 */


#include <sal/config.hpp>
#include <sal/__bits/base64.hpp>
#include <sal/__bits/hex.hpp>
#include <sal/error.hpp>
#include <sal/memory.hpp>
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
template <typename Encoding, typename It>
inline size_t max_encoded_size (It first, It last) noexcept
{
  if (first != last)
  {
    return Encoding::max_encoded_size(to_ptr(first), to_end_ptr(first, last));
  }
  return 0;
}


/**
 * Maximum size of output buffer (in bytes) required to encode \a data
 */
template <typename Encoding, typename Data>
inline size_t max_encoded_size (const Data &data) noexcept
{
  using std::cbegin;
  using std::cend;
  return max_encoded_size<Encoding>(cbegin(data), cend(data));
}


/**
 * Encode data in range [\a first, \a last) using \a Encoding. Result is
 * stored into range [\a out, \a out + max_encoded_size)
 *
 * \returns Iterator to one past final byte stored into output buffer.
 */
template <typename Encoding, typename InputIt, typename ForwardIt>
inline ForwardIt encode (InputIt first, InputIt last, ForwardIt out)
  noexcept
{
  if (first != last)
  {
    return reinterpret_cast<ForwardIt>(
      Encoding::encode(to_ptr(first), to_end_ptr(first, last), to_ptr(out))
    );
  }
  return out;
}


/**
 * Encode data in \a data using \a Encoding. Result is stored into range
 * [\a out, \a out + max_encoded_size)
 *
 * \returns Iterator to one past final byte stored into output buffer.
 */
template <typename Encoding, typename Data, typename ForwardIt>
inline ForwardIt encode (const Data &data, ForwardIt out) noexcept
{
  using std::cbegin;
  using std::cend;
  return encode<Encoding>(cbegin(data), cend(data), out);
}


/**
 * Conveniency wrapper for encoding, returning output data wrapped into
 * std::string.
 */
template <typename Encoding, typename It>
inline std::string encode (It first, It last)
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
template <typename Encoding, typename Data>
inline std::string encode (const Data &data)
{
  using std::cbegin;
  using std::cend;
  return encode<Encoding>(cbegin(data), cend(data));
}


/**
 * Maximum size of output buffer (in bytes) required to decode data in range
 * [\a first, \a last). If input range has invalid number of bytes, \a error
 * is set to \c std::errc::message_size and 0 is returned
 */
template <typename Encoding, typename It>
inline size_t max_decoded_size (It first, It last,
  std::error_code &error) noexcept
{
  if (first != last)
  {
    return Encoding::max_decoded_size(
      to_ptr(first), to_end_ptr(first, last), error
    );
  }
  return 0;
}


/**
 * Maximum size of output buffer (in bytes) required to decode \a data. If
 * input range has invalid number of bytes, \a error is set to
 * \c std::errc::message_size and 0 is returned
 */
template <typename Encoding, typename Data>
inline size_t max_decoded_size (const Data &data, std::error_code &error)
  noexcept
{
  using std::cbegin;
  using std::cend;
  return max_decoded_size<Encoding>(cbegin(data), cend(data), error);
}


/**
 * Maximum size of output buffer (in bytes) required to decode data in range
 * [\a first, \a last). If input range has invalid number of bytes, throws an
 * exception \c std::system_error.
 */
template <typename Encoding, typename It>
inline size_t max_decoded_size (It first, It last)
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
template <typename Encoding, typename Data>
inline size_t max_decoded_size (const Data &data)
{
  return max_decoded_size<Encoding>(data, throw_on_error("max_decoded_size"));
}


/**
 * Decode data in range [\a first, \a last), storing output into buffer
 * starting with \a out. Output buffer should have room for at least
 * max_decoded_size() bytes.
 *
 * \returns Iterator to one past final byte stored in output buffer. On
 * decoding failure, sets \a error to std::errc::illegal_byte_sequence and
 * returned iterator has undefined value.
 */
template <typename Encoding, typename InputIt, typename ForwardIt>
inline ForwardIt decode (InputIt first, InputIt last, ForwardIt out,
  std::error_code &error) noexcept
{
  if (first != last)
  {
    return reinterpret_cast<ForwardIt>(
      Encoding::decode(to_ptr(first), to_end_ptr(first, last), to_ptr(out), error)
    );
  }
  return out;
}


/**
 * Decode data in range [\a first, \a last), storing output into buffer
 * starting with \a out. Output buffer should have room for at least
 * max_decoded_size() bytes.
 *
 * \returns Iterator to one past final byte stored in output buffer. On
 * decoding failure, throws std::system_error
 */
template <typename Encoding, typename InputIt, typename ForwardIt>
inline ForwardIt decode (InputIt first, InputIt last, ForwardIt out)
{
  return decode<Encoding>(first, last, out, throw_on_error("decode"));
}


/**
 * Decode \a data, storing output into buffer starting with \a out. Output
 * buffer should have room for at least max_decoded_size() bytes.
 *
 * \returns Iterator to one past final byte stored in output buffer. On
 * decoding failure, sets \a error to std::errc::illegal_byte_sequence and
 * returned iterator has undefined value.
 */
template <typename Encoding, typename Data, typename ForwardIt>
inline ForwardIt decode (const Data &data, ForwardIt out,
  std::error_code &error) noexcept
{
  using std::cbegin;
  using std::cend;
  return decode<Encoding>(cbegin(data), cend(data), out, error);
}


/**
 * Decode \a data, storing output into buffer starting with \a out. Output
 * buffer should have room for at least max_decoded_size() bytes.
 *
 * \returns Iterator to one past final byte stored in output buffer. On
 * decoding failure, throws std::system_error
 */
template <typename Encoding, typename Data, typename ForwardIt>
inline ForwardIt decode (const Data &data, ForwardIt out)
{
  return decode<Encoding>(data, out, throw_on_error("decode"));
}


/**
 * Conveniency wrapper for decoding, returning output data wrapped into
 * std::vector<uint8_t>.
 */
template <typename Encoding, typename It>
inline std::vector<uint8_t> decode (It first, It last, std::error_code &error)
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
template <typename Encoding, typename It>
inline std::vector<uint8_t> decode (It first, It last)
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
template <typename Encoding, typename Data>
inline std::vector<uint8_t> decode (const Data &data, std::error_code &error)
  noexcept
{
  using std::cbegin;
  using std::cend;
  return decode<Encoding>(cbegin(data), cend(data), error);
}


/**
 * Conveniency wrapper for decoding, returning output data wrapped into
 * std::vector<uint8_t>.
 */
template <typename Encoding, typename Data>
inline std::vector<uint8_t> decode (const Data &data)
{
  using std::cbegin;
  using std::cend;
  return decode<Encoding>(cbegin(data), cend(data));
}


__sal_end
