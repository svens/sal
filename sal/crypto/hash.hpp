#pragma once

/**
 * \file sal/crypto/hash.hpp
 * Cryptographic hash functions
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/digest.hpp>
#include <sal/error.hpp>


__sal_begin


namespace crypto {


using md5 = __bits::md5_t;
using sha1 = __bits::sha1_t;
using sha256 = __bits::sha256_t;
using sha384 = __bits::sha384_t;
using sha512 = __bits::sha512_t;


template <typename Digest>
class hash_t
{
public:

  static constexpr size_t digest_size () noexcept
  {
    return __bits::digest_size_v<Digest>;
  }


  hash_t ();

  hash_t (const hash_t &) = default;
  hash_t &operator= (const hash_t &) = default;
  hash_t (hash_t &&) = default;
  hash_t &operator= (hash_t &&) = default;


  template <typename Ptr>
  void update (const Ptr &data)
  {
    update(data.data(), data.size());
  }


  template <typename Ptr>
  void finish (const Ptr &result)
  {
    sal_throw_if(result.size() < digest_size());
    finish(result.data(), result.size());
  }


  template <typename DataPtr, typename ResultPtr>
  static void one_shot (const DataPtr &data, const ResultPtr &result)
  {
    sal_throw_if(result.size() < digest_size());
    one_shot(data.data(), data.size(), result.data(), result.size());
  }


private:

  typename Digest::hash_t ctx_{};

  void update (const void *data, size_t size);
  void finish (void *result, size_t size);

  static void one_shot (
    const void *data, size_t data_size,
    void *result, size_t result_size
  );
};


} // namespace crypto


__sal_end
