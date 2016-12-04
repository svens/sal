#pragma once

/**
 * \file sal/builtins.hpp
 * Compiler builtins
 */

#include <sal/config.hpp>
#if defined(_MSC_VER)
  #include <intrin.h>
#endif


__sal_begin


/**
 * \def sal_likely(expr)
 * Wrapper for g++ __builtin_expect. Use as little as possible, prefer
 * profiling instead.
 */

/**
 * \def sal_unlikely(expr)
 * \copydoc sal_likely
 */


#if defined(__GNUC__)
  #define sal_likely(expr) (__builtin_expect(!!(expr), 1))
  #define sal_unlikely(expr) (__builtin_expect((expr), 0))
#else
  #define sal_likely(expr) (expr)
  #define sal_unlikely(expr) (expr)
#endif


/**
 * \def sal_clz(value)
 * Count number of leading zero bits in \a value. If \a value is zero, returns
 * undefined value.
 */

#if defined(__GNUC__)
  #define sal_clz(value) (__builtin_clzll(value))
#elif defined(_MSC_VER)
  #define sal_clz(value) (sal::__bits::clz(value))
#else
  #error Unsupported platform
  #define sal_clz(value)
#endif


namespace __bits {

#if defined(_MSC_VER)

  #if defined(_WIN64)

    #pragma intrinsic(_BitScanReverse64)
    inline unsigned clz (uint64_t value) noexcept
    {
      unsigned long r = 0;
      _BitScanReverse64(&r, value);
      return 63 - r;
    }

  #else // _WIN64

    #pragma intrinsic(_BitScanReverse)
    inline unsigned clz (uint64_t value) noexcept
    {
      unsigned long r = 0;
      if (_BitScanReverse(&r, static_cast<uint32_t>(value >> 32)))
      {
        return 63 - (r + 32);
      }
      _BitScanReverse(&r, static_cast<uint32_t>(value));
      return 63 - r;
    }

  #endif // _WIN64

#endif // _MSC_VER


inline size_t digits (uint64_t v) noexcept
{
  // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10

  static constexpr uint64_t pow10[] =
  {
    0ULL,
    10ULL,
    100ULL,
    1000ULL,
    10000ULL,
    100000ULL,
    1000000ULL,
    10000000ULL,
    100000000ULL,
    1000000000ULL,
    10000000000ULL,
    100000000000ULL,
    1000000000000ULL,
    10000000000000ULL,
    100000000000000ULL,
    1000000000000000ULL,
    10000000000000000ULL,
    100000000000000000ULL,
    1000000000000000000ULL,
    10000000000000000000ULL,
  };

  size_t t = (64 - sal_clz(v | 1)) * 1233 >> 12;
  return t - (v < pow10[t]) + 1;
}


} // namespace __bits


__sal_end
