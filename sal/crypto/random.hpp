#pragma once

/**
 * \file sal/crypto/random.hpp
 * Cryptographic random generation
 */


#include <sal/config.hpp>
#include <memory>


__sal_begin


namespace crypto {


namespace __bits {

void random (void *data, size_t size);

} // namespace __bits


/**
 * Fill \a data with cryptographically strong random bytes suitable for
 * cryptographic keys, nonces, etc.
 */
template <typename Ptr>
inline void random (const Ptr &data)
{
  __bits::random(data.data(), data.size());
}


/**
 * Fill range [\a first, \a last) with cryptographically strong random bytes
 * suitable for cryptographic keys, nonces, etc.
 */
template <typename It>
inline void random (It first, It last)
{
  __bits::random(
    reinterpret_cast<uint8_t *>(std::addressof(*first)),
    (last - first) * sizeof(*first)
  );
}


} // namespace crypto


__sal_end
