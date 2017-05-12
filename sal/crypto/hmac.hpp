#pragma once

/**
 * \file sal/crypto/hmac.hpp
 * Cryptographic HMAC functions
 */


#include <sal/config.hpp>
#include <sal/crypto/hash.hpp>
#include <sal/error.hpp>


__sal_begin


namespace crypto {


template <typename Digest>
class hmac_t
{
public:

  static constexpr size_t digest_size () noexcept
  {
    return __bits::digest_size_v<Digest>;
  }


  hmac_t ()
    : hmac_t(nullptr, 0U)
  {}


  template <typename Ptr>
  hmac_t (const Ptr &key)
    : hmac_t(key.data(), key.size())
  {}


  hmac_t (const hmac_t &) = default;
  hmac_t &operator= (const hmac_t &) = default;
  hmac_t (hmac_t &&) = default;
  hmac_t &operator= (hmac_t &&) = default;


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


  template <typename KeyPtr, typename DataPtr, typename ResultPtr>
  static void one_shot (const KeyPtr &key,
    const DataPtr &data,
    const ResultPtr &result)
  {
    sal_throw_if(result.size() < digest_size());
    one_shot(key.data(), key.size(),
      data.data(), data.size(),
      result.data(), result.size()
    );
  }


  template <typename DataPtr, typename ResultPtr>
  static void one_shot (const DataPtr &data, const ResultPtr &result)
  {
    sal_throw_if(result.size() < digest_size());
    one_shot(nullptr, 0U,
      data.data(), data.size(),
      result.data(), result.size()
    );
  }


private:

  typename Digest::hmac_t ctx_;

  hmac_t (const void *key, size_t size);

  void update (const void *data, size_t size);
  void finish (void *result, size_t size);

  static void one_shot (
    const void *key, size_t key_size,
    const void *data, size_t data_size,
    void *result, size_t result_size
  );
};


} // namespace crypto


__sal_end
