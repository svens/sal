#pragma once

/**
 * \file sal/crypto/key.hpp
 * Asymmetrical cryptography public and private keys.
 *
 * \see https://en.wikipedia.org/wiki/Public-key_cryptography
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/digest.hpp>
#include <sal/crypto/__bits/x509.hpp>
#include <sal/crypto/error.hpp>
#include <vector>


__sal_begin


namespace crypto {


/** Key algorithms */
enum class key_algorithm
{
  opaque,       ///< Unspecified algorithm type
  rsa,          ///< RSA algorithm
};


/**
 * Public key portion of asymmetric key pair.
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
    std::swap(algorithm_, that.algorithm_);
    std::swap(block_size_, that.block_size_);
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


  /**
   * Return key algorithm type. Result is undefined if key is not set.
   */
  key_algorithm algorithm () const noexcept
  {
    return algorithm_;
  }


  /**
   * Return block length associated with key. Result is undefined if key is
   * not set.
   */
  size_t block_size () const noexcept
  {
    return block_size_;
  }


  /**
   * Verify if signature [\a signature_first, \a signature_last) is valid for
   * [\a first, \a last) signed with corresponding private key using \a Digest
   * algorithm.
   *
   * \returns True if \a signature is valid
   */
  template <typename Digest, typename DataIt, typename SignatureIt>
  bool verify_signature (Digest,
    DataIt first, DataIt last,
    SignatureIt signature_first, SignatureIt signature_last,
    std::error_code &error) noexcept
  {
    return verify_signature(__bits::digest_type_v<Digest>,
      first != last ? to_ptr(first) : nullptr,
      range_size(first, last),
      to_ptr(signature_first),
      range_size(signature_first, signature_last),
      error
    );
  }


  /**
   * Verify if signature [\a signature_first, \a signature_last) is valid for
   * [\a first, \a last) signed with corresponding private key using \a Digest
   * algorithm.
   *
   * \returns True if \a signature is valid
   */
  template <typename Digest, typename DataIt, typename SignatureIt>
  bool verify_signature (Digest digest,
    DataIt first, DataIt last,
    SignatureIt signature_first, SignatureIt signature_last)
  {
    return verify_signature(digest,
      first, last, signature_first, signature_last,
      throw_on_error("public_key::verify_signature")
    );
  }


  /**
   * Verify if \a signature is valid for \a data signed with corresponding
   * private key using \a digest.
   * \returns True if \a signature is valid
   */
  template <typename Digest, typename Data, typename Signature>
  bool verify_signature (Digest digest,
    const Data &data,
    const Signature &signature,
    std::error_code &error) noexcept
  {
    using std::cbegin;
    using std::cend;
    return verify_signature(digest,
      cbegin(data), cend(data),
      cbegin(signature), cend(signature),
      error
    );
  }


  /**
   * Verify if \a signature is valid for \a data signed with corresponding
   * private key using \a digest.
   * \returns True if \a signature is valid
   */
  template <typename Digest, typename Data, typename Signature>
  bool verify_signature (Digest digest,
    const Data &data,
    const Signature &signature) noexcept
  {
    using std::cbegin;
    using std::cend;
    return verify_signature(digest, data, signature,
      throw_on_error("public_key::verify_signature")
    );
  }


private:

  __bits::public_key_t impl_{};
  key_algorithm algorithm_{};
  size_t block_size_{};

  public_key_t (__bits::public_key_t &&that) noexcept;

  bool verify_signature (size_t digest_type,
    const void *data, size_t data_size,
    const void *signature, size_t signature_size,
    std::error_code &error
  ) noexcept;

  friend class certificate_t;
};


/**
 * Private key portion of asymmetric key pair.
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
    std::swap(algorithm_, that.algorithm_);
    std::swap(block_size_, that.block_size_);
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


  /**
   * Return private key algorithm type. Result is undefined if key is not set.
   */
  key_algorithm algorithm () const noexcept
  {
    return algorithm_;
  }


  /**
   * Return block length associated with key. Result is undefined if key is
   * not set.
   */
  size_t block_size () const noexcept
  {
    return block_size_;
  }


  /**
   * Sign data in [\a first, \a last) using \a Digest algorithm and write
   * result into [\a signature_first, \a signature_last).
   *
   * \returns Size of signature or undefined value on error.
   */
  template <typename Digest, typename DataIt, typename SignatureIt>
  size_t sign (Digest,
    DataIt first, DataIt last,
    SignatureIt signature_first, SignatureIt signature_last,
    std::error_code &error) noexcept
  {
    return sign(__bits::digest_type_v<Digest>,
      first != last ? to_ptr(first) : nullptr,
      range_size(first, last),
      to_ptr(signature_first),
      range_size(signature_first, signature_last),
      error
    );
  }


  /**
   * Sign data in [\a first, \a last) using \a digest algorithm and write
   * result into [\a signature_first, \a signature_last).
   *
   * \returns Size of signature
   */
  template <typename Digest, typename DataIt, typename SignatureIt>
  size_t sign (Digest digest,
    DataIt first, DataIt last,
    SignatureIt signature_first, SignatureIt signature_last)
  {
    return sign(digest, first, last, signature_first, signature_last,
      throw_on_error("private_key::sign")
    );
  }


  /**
   * Sign \a data using \a digest algorithm and return signature as vector.
   * \throws std::system_error on signing failure
   * \throws std::bad_alloc on vector allocation failure
   */
  template <typename Digest, typename Data>
  std::vector<uint8_t> sign (Digest digest, const Data &data)
  {
    std::vector<uint8_t> result(block_size());
    using std::cbegin;
    using std::cend;
    sign(digest, cbegin(data), cend(data), result.begin(), result.end());
    return result;
  }


private:

  __bits::private_key_t impl_{};
  key_algorithm algorithm_{};
  size_t block_size_{};

  private_key_t (__bits::private_key_t &&that) noexcept;

  size_t sign (size_t digest_type,
    const void *data, size_t data_size,
    void *signature, size_t signature_size,
    std::error_code &error
  ) noexcept;

  friend class certificate_t;
};


} // namespace crypto


__sal_end
