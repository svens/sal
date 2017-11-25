#pragma once

/**
 * \file sal/crypto/hash.hpp
 * Cryptographic hash functions
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/digest.hpp>
#include <sal/error.hpp>
#include <sal/memory.hpp>
#include <array>


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
   * Number of bytes in hash digest result.
   */
  static constexpr size_t digest_size = __bits::digest_size_v<Algorithm>;


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
   * Add region [\a first, \a last) into hasher.
   */
  template <typename It>
  void update (It first, It last)
  {
    if (first != last)
    {
      update(to_ptr(first), range_size(first, last));
    }
  }


  /**
   * Add more \a data into hasher.
   */
  template <typename Data>
  void update (const Data &data)
  {
    using std::cbegin;
    using std::cend;
    update(cbegin(data), cend(data));
  }


  /**
   * Calculate hash of previously added data and store into region starting at
   * \a first. If size of region [\a first, \a last) is less than
   * digest_size, throw std::logic_error
   */
  template <typename It>
  void finish (It first, It last)
  {
    auto result_size = range_size(first, last);
    sal_throw_if(result_size < digest_size);
    finish(to_ptr(first), result_size);
  }


  /**
   * Calculate hash of previously added data and return result as array.
   */
  std::array<uint8_t, digest_size> finish ()
  {
    std::array<uint8_t, digest_size> result;
    finish(result.begin(), result.end());
    return result;
  }


  /**
   * Calculate hash of range [\a first, \a last) and store result into range
   * [\a digest_first, \a digest_last).
   * \throws std::logic_error if size of [\a digest_first, \a digest_last) <
   * digest_size
   */
  template <typename It, typename DigestIt>
  static void one_shot (It first, It last,
    DigestIt digest_first, DigestIt digest_last)
  {
    auto result_size = range_size(digest_first, digest_last);
    sal_throw_if(result_size < digest_size);
    if (first != last)
    {
      one_shot(to_ptr(first), range_size(first, last),
        to_ptr(digest_first), result_size
      );
    }
    else
    {
      one_shot(to_ptr(digest_first), 0, to_ptr(digest_first), result_size);
    }
  }


  /**
   * Calculate hash of range [\a first, \a last) and return result as array.
   */
  template <typename It>
  static std::array<uint8_t, digest_size> one_shot (It first, It last)
  {
    std::array<uint8_t, digest_size> result;
    one_shot(first, last, result.begin(), result.end());
    return result;
  }


  /**
   * Calculate hash of \a data and return result as array.
   */
  template <typename Data>
  static std::array<uint8_t, digest_size> one_shot (const Data &data)
  {
    using std::cbegin;
    using std::cend;
    return one_shot(cbegin(data), cend(data));
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
