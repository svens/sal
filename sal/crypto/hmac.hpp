#pragma once

/**
 * \file sal/crypto/hmac.hpp
 * Cryptographic HMAC functions
 */


#include <sal/config.hpp>
#include <sal/crypto/hash.hpp>
#include <sal/error.hpp>
#include <sal/memory.hpp>


__sal_begin


namespace crypto {


/**
 * Keyed-hash message authentication code
 * (<a href="https://en.wikipedia.org/wiki/Cryptographic_hash_function">HMAC</a>)
 * using \a Algorithm.
 *
 * Usage of this class is similar to hash_t.
 */
template <typename Algorithm>
class hmac_t
{
public:

  /**
   * Return number of bytes in HMAC digest.
   */
  static constexpr size_t digest_size () noexcept
  {
    return __bits::digest_size_v<Algorithm>;
  }


  /**
   * Initialize HMAC object using empty key.
   */
  hmac_t ()
    : hmac_t(nullptr, 0U)
  {}


  /**
   * Initialize HMAC object using key in range [\a first, \a last).
   */
  template <typename It>
  hmac_t (It first, It last)
    : hmac_t(to_ptr(first), range_size(first, last))
  {}


  /**
   * Initialize HMAC object using \a key.
   */
  template <typename Ptr>
  hmac_t (const Ptr &key)
    : hmac_t(key.data(), key.size())
  {}


  /**
   * Create this as copy of \a that
   */
  hmac_t (const hmac_t &that) = default;


  /**
   * Copy state of \a that into this, overriding current state.
   */
  hmac_t &operator= (const hmac_t &that) = default;


  /**
   * Create this from \a that by moving it's state into this.
   * It is undefined behaviour to use \a that without re-initializing it
   * after move.
   */
  hmac_t (hmac_t &&that) = default;


  /**
   * Drop current state of this and move state of \a that into this.
   * It is undefined behaviour to use \a that without re-initializing it
   * after move.
   */
  hmac_t &operator= (hmac_t &&that) = default;


  /**
   * Add region [\a first, \a last) into hasher.
   */
  template <typename It>
  void update (It first, It last)
  {
    update(to_ptr(first), range_size(first, last));
  }


  /**
   * Add more \a data into hasher.
   */
  template <typename Ptr>
  void update (const Ptr &data)
  {
    update(data.data(), data.size());
  }


  /**
   * Calculate HMAC of previously added data and store into region starting at
   * \a first. If size of region [\a first, \a last) is less than
   * digest_size(), throw std::logic_error
   */
  template <typename It>
  void finish (It first, It last)
  {
    sal_throw_if(range_size(first, last) < digest_size());
    finish(to_ptr(first), range_size(first, last));
  }


  /**
   * Calculate HMAC of previously added data and store it into \a digest.
   * \throws std::logic_error if digest.size() < digest_size()
   */
  template <typename Ptr>
  void finish (const Ptr &digest)
  {
    sal_throw_if(digest.size() < digest_size());
    finish(digest.data(), digest.size());
  }


  /**
   * Calculate HMAC of \a data using \a key and store result into \a digest.
   * \throws std::logic_error if digest.size() < digest_size()
   */
  template <typename KeyPtr, typename DataPtr, typename DigestPtr>
  static void one_shot (const KeyPtr &key,
    const DataPtr &data,
    const DigestPtr &digest)
  {
    sal_throw_if(digest.size() < digest_size());
    one_shot(key.data(), key.size(),
      data.data(), data.size(),
      digest.data(), digest.size()
    );
  }


  /**
   * Calculate HMAC of \a data without key and store result into \a digest.
   * \throws std::logic_error if digest.size() < digest_size()
   */
  template <typename DataPtr, typename DigestPtr>
  static void one_shot (const DataPtr &data, const DigestPtr &digest)
  {
    sal_throw_if(digest.size() < digest_size());
    one_shot(nullptr, 0U,
      data.data(), data.size(),
      digest.data(), digest.size()
    );
  }


private:

  typename Algorithm::hmac_t ctx_;

  hmac_t (const void *key, size_t size);

  void update (const void *data, size_t size);
  void finish (void *digest, size_t size);

  static void one_shot (
    const void *key, size_t key_size,
    const void *data, size_t data_size,
    void *digest, size_t digest_length
  );
};


} // namespace crypto


__sal_end
