#pragma once

/**
 * \file sal/crypto_hash.hpp
 * Cryptographic hash functions
 */


#include <sal/config.hpp>
#include <sal/assert.hpp>
#include <sal/__bits/crypto_hash.hpp>


__sal_begin


using md2 = __bits::md2_t;
using md4 = __bits::md4_t;
using md5 = __bits::md5_t;
using sha_1 = __bits::sha_1_t;
using sha_256 = __bits::sha_256_t;
using sha_384 = __bits::sha_384_t;
using sha_512 = __bits::sha_512_t;


template <typename T>
class crypto_hash_t
{
public:

  static constexpr size_t size () noexcept
  {
    return T::size();
  }


  template <typename Ptr>
  void add (const Ptr &ptr) noexcept
  {
    impl_.add(ptr);
  }


  template <typename Ptr>
  void finish (Ptr &ptr)
  {
    sal_assert(ptr.size() >= impl_.size());
    impl_.finish(ptr);
  }


private:

  T impl_;
};


__sal_end
