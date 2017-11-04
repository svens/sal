#pragma once

/**
 * \file sal/hash.hpp
 * Various non-cryptographic hash functions
 */


#include <sal/config.hpp>


__sal_begin


/**
 * Hash two 64bit values into single 64bit value. This implementation is
 * copied from Google's CityHash. See ThirdPartySources.txt for copyright
 * notices.
 */
constexpr uint64_t hash_128_to_64 (uint64_t h, uint64_t l) noexcept
{
  constexpr uint64_t mul = 0x9ddfea08eb382d69ULL;
  uint64_t a = (l ^ h) * mul;
  a ^= (a >> 47);
  uint64_t b = (h ^ a) * mul;
  b ^= (b >> 47);
  return b * mul;
}


/**
 * Fowler-Noll-Vo hashing function for 64bit result.
 *
 * This implementation is copied from it's corresponding homepage at
 * http://www.isthe.com/chongo/tech/comp/fnv/.
 *
 * See ThirdPartySources.txt for copyright notices.
 */
template <typename It>
constexpr uint64_t fnv_1a_64 (It first, It last, uint64_t h = 0xcbf29ce484222325ULL)
  noexcept
{
  while (first != last)
  {
    h ^= *first++;
    h += (h << 1) + (h << 4) + (h << 5) + (h << 7) + (h << 8) + (h << 40);
  }
  return h;
}


__sal_end
