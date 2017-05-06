#pragma once

/**
 * \file sal/crypto/hash.hpp
 * Cryptographic hash functions
 */


#include <sal/config.hpp>
#include <sal/error.hpp>
#include <sal/crypto/__bits/hash_algorithm.hpp>


__sal_begin


namespace crypto {


using md5 = __bits::md5_t;
using sha1 = __bits::sha1_t;
using sha256 = __bits::sha256_t;
using sha384 = __bits::sha384_t;
using sha512 = __bits::sha512_t;


template <typename T>
class hash_t
{
public:

  static constexpr size_t digest_size () noexcept
  {
    return T::digest_size;
  }


  hash_t () = default;

  hash_t (hash_t &&) noexcept = default;
  hash_t &operator= (hash_t &&) noexcept = default;

  hash_t (const hash_t &) = delete;
  hash_t &operator= (const hash_t &) = delete;


  template <typename Ptr>
  void update (const Ptr &data)
  {
    impl_.update(data.data(), data.size());
  }


  template <typename Ptr>
  void finish (const Ptr &result)
  {
    sal_throw_if(result.size() < digest_size());
    impl_.finish(result.data());
  }


private:

  typename T::hash_t impl_{};
};


} // namespace crypto


__sal_end
