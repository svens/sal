#pragma once

/**
 * \file sal/crypto/hash.hpp
 * Cryptographic hash functions
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/digest.hpp>
#include <sal/error.hpp>


__sal_begin


/// Cryptography primitives
namespace crypto {


/// \{
/// Cryptographic hash algorithm
using md5 = __bits::md5_t;
using sha1 = __bits::sha1_t;
using sha256 = __bits::sha256_t;
using sha384 = __bits::sha384_t;
using sha512 = __bits::sha512_t;
/// \}


/**
 * One way <a href="https://en.wikipedia.org/wiki/Cryptographic_hash_function">cryptographic hashing</a>
 * using \a Algorithm.
 *
 * If full data is already in continuous memory region, call static one_shot()
 * to calculate digest immediately.
 *
 * If data is not completely stored in memory, instantiate this class and add
 * new data in one or more calls to update(). Once full data has been fed into
 * hasher, call finish() to fetch calculated digest. Application can
 * immediately reuse instance after call to finish(), it is re-initialized
 * internally.
 */
template <typename Algorithm>
class hash_t
{
public:

  /**
   * Return number of bytes in hash digest.
   */
  static constexpr size_t digest_size () noexcept
  {
    return __bits::digest_size_v<Algorithm>;
  }


  /**
   * Initialize hasher object
   */
  hash_t ();


  /**
   * Create this as copy of \a that
   */
  hash_t (const hash_t &that) = default;


  /**
   * Copy state of \a that into this, overriding current state.
   */
  hash_t &operator= (const hash_t &that) = default;


  /**
   * Create this from \a that by moving it's state into this.
   * It is undefined behaviour to use \a that without re-initializing it
   * after move.
   */
  hash_t (hash_t &&that) = default;


  /**
   * Drop current state of this and move state of \a that into this.
   * It is undefined behaviour to use \a that without re-initializing it
   * after move.
   */
  hash_t &operator= (hash_t &&that) = default;


  /**
   * Add more \a data into hasher.
   */
  template <typename Ptr>
  void update (const Ptr &data)
  {
    update(data.data(), data.size());
  }


  /**
   * Calculate hash of previously added data and store it into \a digest.
   * \throws std::logic_error if digest.size() < digest_size()
   */
  template <typename Ptr>
  void finish (const Ptr &digest)
  {
    sal_throw_if(digest.size() < digest_size());
    finish(digest.data(), digest.size());
  }


  /**
   * Calculate hash of \a data and store result into \a digest.
   * \throws std::logic_error if digest.size() < digest_size()
   */
  template <typename DataPtr, typename DigestPtr>
  static void one_shot (const DataPtr &data, const DigestPtr &digest)
  {
    sal_throw_if(digest.size() < digest_size());
    one_shot(data.data(), data.size(), digest.data(), digest.size());
  }


private:

  typename Algorithm::hash_t ctx_{};

  void update (const void *data, size_t size);
  void finish (void *digest, size_t size);

  static void one_shot (
    const void *data, size_t data_size,
    void *digest, size_t digest_length
  );
};


} // namespace crypto


__sal_end
