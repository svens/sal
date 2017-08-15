#pragma once

/**
 * \file sal/crypto/key.hpp
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/x509.hpp>


__sal_begin


namespace crypto {


class public_key_t
{
public:

  public_key_t () = default;
  public_key_t (public_key_t &&) = default;
  public_key_t &operator= (public_key_t &&) = default;

  public_key_t (const public_key_t &) = delete;
  public_key_t &operator= (const public_key_t &) = delete;


  /**
   */
  void swap (public_key_t &that) noexcept
  {
    impl_.swap(that.impl_);
  }


  /**
   */
  bool is_null () const noexcept
  {
    return impl_.is_null();
  }


  /**
   */
  explicit operator bool () const noexcept
  {
    return !is_null();
  }


private:

  __bits::public_key_t impl_{};

  public_key_t (__bits::public_key_t &&that) noexcept
    : impl_(std::move(that))
  {}

  friend class certificate_t;
};


} // namespace crypto


__sal_end
