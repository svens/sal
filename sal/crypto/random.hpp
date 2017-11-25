#pragma once

/**
 * \file sal/crypto/random.hpp
 * Cryptographic random generation
 */


#include <sal/config.hpp>
#include <sal/memory.hpp>


__sal_begin


namespace crypto {


namespace __bits {

void random (void *data, size_t size);

} // namespace __bits


/**
 * Fill range [\a first, \a last) with cryptographically strong random bytes
 * suitable for cryptographic keys, nonces, etc.
 */
template <typename It>
inline void random (It first, It last)
{
  if (first != last)
  {
    __bits::random(to_ptr(first), range_size(first, last));
  }
}


/**
 * Fill \a data with cryptographically strong random bytes suitable for
 * cryptographic keys, nonces, etc.
 */
template <typename Data>
inline void random (Data &data)
{
  using std::begin;
  using std::end;
  random(begin(data), end(data));
}


} // namespace crypto


__sal_end
