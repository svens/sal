#pragma once

/**
 * \file sal/crypto/key.hpp
 * Asymmetrical cryptography public and private keys.
 *
 * \see https://en.wikipedia.org/wiki/Public-key_cryptography
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/x509.hpp>


__sal_begin


namespace crypto {


/**
 * Public key.
 */
class public_key_t
{
public:

  public_key_t (const public_key_t &) = delete;
  public_key_t &operator= (const public_key_t &) = delete;

  /// Construct null key
  public_key_t () = default;

  /// Construct new key from other key, zeroing other key
  public_key_t (public_key_t &&) = default;

  /// Acquire key from other key, zeroing other key
  public_key_t &operator= (public_key_t &&) = default;


  /**
   * Swap \a this with \a that.
   */
  void swap (public_key_t &that) noexcept
  {
    impl_.swap(that.impl_);
  }


  /**
   * Return true if \a this represents null key (ie no key)
   */
  bool is_null () const noexcept
  {
    return impl_.is_null();
  }


  /**
   * Return true if \a this is valid key.
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


/**
 * Private key.
 */
class private_key_t
{
public:

  private_key_t (const private_key_t &) = delete;
  private_key_t &operator= (const private_key_t &) = delete;

  /// Construct null key
  private_key_t () = default;

  /// Construct new key from other key, zeroing other key
  private_key_t (private_key_t &&) = default;

  /// Acquire key from other key, zeroing other key
  private_key_t &operator= (private_key_t &&) = default;


  /**
   * Swap \a this with \a that.
   */
  void swap (private_key_t &that) noexcept
  {
    impl_.swap(that.impl_);
  }


  /**
   * Return true if \a this represents null key (ie no key)
   */
  bool is_null () const noexcept
  {
    return impl_.is_null();
  }


  /**
   * Return true if \a this is valid key.
   */
  explicit operator bool () const noexcept
  {
    return !is_null();
  }


private:

  __bits::private_key_t impl_{};

  private_key_t (__bits::private_key_t &&that) noexcept
    : impl_(std::move(that))
  {}

  friend class certificate_t;
};


} // namespace crypto


__sal_end
