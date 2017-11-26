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
   * Number of bytes in hash digest result.
   */
  static constexpr size_t digest_size = __bits::digest_size_v<Algorithm>;


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
  template <typename Data>
  hmac_t (const Data &key)
    : hmac_t(std::cbegin(key), std::cend(key))
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
   * Calculate HMAC of previously added data and store into region starting at
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
   * Calculate HMAC of previously added data and return result as array.
   */
  std::array<uint8_t, digest_size> finish ()
  {
    std::array<uint8_t, digest_size> result;
    finish(result.begin(), result.end());
    return result;
  }


  /**
   * Calculate HMAC over [\a data_first, \a data_last) without key and write
   * result into [\a digest_first, \a digest_last).
   *
   * \throws std::logic_error if area [\a digest_first, \a digest_last) is
   * less than digest_size
   */
  template <typename DataIt, typename DigestIt>
  static void one_shot (DataIt data_first,
    DataIt data_last,
    DigestIt digest_first,
    DigestIt digest_last)
  {
    auto result_size = range_size(digest_first, digest_last);
    sal_throw_if(result_size < digest_size);
    auto result = to_ptr(digest_first);
    if (data_first != data_last)
    {
      auto data = to_ptr(data_first);
      auto data_size = range_size(data_first, data_last);
      one_shot(nullptr, 0, data, data_size, result, result_size);
    }
    else
    {
      one_shot(nullptr, 0, nullptr, 0, result, result_size);
    }
  }


  /**
   * Calculate HMAC over [\a data_first, \a data_last) using key in
   * [\a key_first, \a key_last) and write result into [\a digest_first,
   * \a digest_last).
   *
   * \throws std::logic_error if area [\a digest_first, \a digest_last) is
   * less than digest_size
   */
  template <typename KeyIt, typename DataIt, typename DigestIt>
  static void one_shot (KeyIt key_first,
    KeyIt key_last,
    DataIt data_first,
    DataIt data_last,
    DigestIt digest_first,
    DigestIt digest_last)
  {
    auto result_size = range_size(digest_first, digest_last);
    sal_throw_if(result_size < digest_size);
    auto result = to_ptr(digest_first);
    auto key = to_ptr(key_first);
    auto key_size = range_size(key_first, key_last);
    if (data_first != data_last)
    {
      auto data = to_ptr(data_first);
      auto data_size = range_size(data_first, data_last);
      one_shot(key, key_size, data, data_size, result, result_size);
    }
    else
    {
      one_shot(key, key_size, nullptr, 0, result, result_size);
    }
  }


  /**
   * Calculate HMAC over \a data without key and return result as std::array
   */
  template <typename Data>
  static std::array<uint8_t, digest_size> one_shot (const Data &data)
  {
    std::array<uint8_t, digest_size> result;
    using std::cbegin;
    using std::cend;
    one_shot(cbegin(data), cend(data), result.begin(), result.end());
    return result;
  }


  /**
   * Calculate HMAC over \a data with \a key and return result as std::array
   */
  template <typename Key, typename Data>
  static std::array<uint8_t, digest_size> one_shot (const Key &key,
    const Data &data)
  {
    std::array<uint8_t, digest_size> result;
    using std::cbegin;
    using std::cend;
    one_shot(
      cbegin(key), cend(key),
      cbegin(data), cend(data),
      result.begin(), result.end()
    );
    return result;
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
