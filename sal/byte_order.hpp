#pragma once

/**
 * \file sal/byte_order.hpp
 * Host / network byte order conversions
 */

#include <sal/config.hpp>


#if defined(__GNUC__) || defined(__clang__)
  #define __sal_little_endian   __ORDER_LITTLE_ENDIAN__
  #define __sal_big_endian      __ORDER_BIG_ENDIAN__
  #define __sal_byte_order      __BYTE_ORDER__
#elif defined(_WIN32) || defined(_WIN64)
  #define __sal_little_endian   0
  #define __sal_big_endian      1
  #define __sal_byte_order      __sal_little_endian
#else
  #error Unsupported platform
#endif


__sal_begin


/**
 * Byte ordering.
 */
enum class endian
{
  little = __sal_little_endian, ///< Little endian
  big = __sal_big_endian,       ///< Big endian
  native = __sal_byte_order     ///< Native endian (either little or big)
};


/**
 * Convert 16-bit \a value from host order to network order.
 */
constexpr inline uint16_t native_to_network_byte_order (uint16_t value)
  noexcept
{
  if constexpr (endian::native == endian::little)
  {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(value);
#else
    return
      ((value & 0x00ff) << 8) |
      ((value & 0xff00) >> 8) ;
#endif
  }
  else
  {
    return value;
  }
}


/**
 * Convert 32-bit \a value from host order to network order.
 */
constexpr inline uint32_t native_to_network_byte_order (uint32_t value)
  noexcept
{
  if constexpr (endian::native == endian::little)
  {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(value);
#else
    return
      ((value & 0x0000'00ff) << 24) |
      ((value & 0x0000'ff00) <<  8) |
      ((value & 0x00ff'0000) >>  8) |
      ((value & 0xff00'0000) >> 24) ;
#endif
  }
  else
  {
    return value;
  }
}


/**
 * Convert 64-bit \a value from host order to network order.
 */
constexpr inline uint64_t native_to_network_byte_order (uint64_t value)
  noexcept
{
  if constexpr (endian::native == endian::little)
  {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(value);
#else
    return
      ((value & 0x0000'0000'0000'00ff) << 56) |
      ((value & 0x0000'0000'0000'ff00) << 40) |
      ((value & 0x0000'0000'00ff'0000) << 24) |
      ((value & 0x0000'0000'ff00'0000) <<  8) |
      ((value & 0x0000'00ff'0000'0000) >>  8) |
      ((value & 0x0000'ff00'0000'0000) >> 24) |
      ((value & 0x00ff'0000'0000'0000) >> 40) |
      ((value & 0xff00'0000'0000'0000) >> 56) ;
#endif
  }
  else
  {
    return value;
  }
}


/**
 * Convert 16-bit \a value from network order to host order.
 */
constexpr inline uint16_t network_to_native_byte_order (uint16_t value)
  noexcept
{
  return native_to_network_byte_order(value);
}


/**
 * Convert 32-bit \a value from network order to host order.
 */
constexpr inline uint32_t network_to_native_byte_order (uint32_t value)
  noexcept
{
  return native_to_network_byte_order(value);
}


/**
 * Convert 64-bit \a value from network order to host order.
 */
constexpr inline uint64_t network_to_native_byte_order (uint64_t value)
  noexcept
{
  return native_to_network_byte_order(value);
}


__sal_end
