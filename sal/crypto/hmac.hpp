#pragma once

/**
 * \file sal/crypto/hmac.hpp
 * Cryptographic HMAC functions
 */


#include <sal/config.hpp>
#include <sal/error.hpp>
#include <sal/crypto/hash.hpp>


__sal_begin


namespace crypto {


template <typename T>
class hmac_t
{
public:

  static constexpr size_t digest_size () noexcept
  {
    return T::digest_size;
  }


  hmac_t () = default;

  template <typename Ptr>
  hmac_t (const Ptr &key)
    : impl_{key.data(), key.size()}
  {}

  hmac_t (hmac_t &&) noexcept = default;
  hmac_t &operator= (hmac_t &&) noexcept = default;

  hmac_t (const hmac_t &) = delete;
  hmac_t &operator= (const hmac_t &) = delete;


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


  template <typename KeyPtr, typename DataPtr, typename ResultPtr>
  static void one_shot (const KeyPtr &key,
    const DataPtr &data,
    const ResultPtr &result)
  {
    sal_throw_if(result.size() < digest_size());
    T::hmac_t::one_shot(key.data(), key.size(),
      data.data(), data.size(),
      result.data()
    );
  }


  template <typename DataPtr, typename ResultPtr>
  static void one_shot (const DataPtr &data, const ResultPtr &result)
  {
    sal_throw_if(result.size() < digest_size());
    T::hmac_t::one_shot(nullptr, 0U,
      data.data(), data.size(),
      result.data()
    );
  }


private:

  typename T::hmac_t impl_{};
};


} // namespace crypto


__sal_end
