#pragma once

/**
 * \file sal/crypto/certificate.hpp
 * Public key certificate in X509 format.
 *
 * \see https://en.wikipedia.org/wiki/Public_key_certificate
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/x509.hpp>
#include <sal/crypto/hash.hpp>
#include <sal/crypto/key.hpp>
#include <sal/crypto/oid.hpp>
#include <sal/char_array.hpp>
#include <sal/time.hpp>
#include <ostream>
#include <vector>


__sal_begin


namespace crypto {


/**
 * Wrapper for platform's native public key certificate.
 *
 * Platforms use following native implementations:
 * - MacOS: Security framework with SecCertificateRef
 * - Linux: OpenSSL with X509
 * - Windows: WinCrypt with PCCERT_CONTEXT
 */
class certificate_t
{
public:

  /**
   * Construct new empty certificate
   */
  certificate_t () = default;

  /**
   * Construct new certificate from other (increase reference count)
   */
  certificate_t (const certificate_t &) = default;

  /**
   * Construct new certificate from other, acquiring native handle and zeroing
   * other instance.
   */
  certificate_t (certificate_t &&) = default;

  /**
   * Assign \a this from other, increasing native handle reference count.
   */
  certificate_t &operator= (const certificate_t &) = default;

  /**
   * Assign \a this from other, zeroing other instance.
   */
  certificate_t &operator= (certificate_t &&) = default;


  /**
   * Swap \a this certificate handle with \a that.
   */
  void swap (certificate_t &that) noexcept
  {
    impl_.swap(that.impl_);
  }


  /**
   * Return true if \a this represents same certificate as \a that.
   */
  bool operator== (const certificate_t &that) const noexcept;


  /**
   * Return true if \a this represents different certificate as \a that.
   */
  bool operator!= (const certificate_t &that) const noexcept
  {
    return !operator==(that);
  }


  /**
   * Import certificates from PKCS12 formatted blob [\a first, \a last).
   * Return leaf certificate and inserts other possible certificates into
   * \a chain (if not \c nullptr). Private key corresponding to leaf
   * certificate is extracted using \a passphrase and assigned to
   * \a private_key if not \c nullptr.
   *
   * On success, error is cleared. On failure returns null certificate and
   * sets \a error.
   */
  static certificate_t import_pkcs12 (
    const uint8_t *first, const uint8_t *last,
    const std::string &passphrase,
    private_key_t *private_key,
    std::vector<certificate_t> *chain,
    std::error_code &error
  ) noexcept;


  /**
   * Load and construct new certificate from DER encoded blob in
   * [\a first, \a last). On failure return null certificate and set
   * \a error.
   */
  static certificate_t from_der (const uint8_t *first, const uint8_t *last,
    std::error_code &error
  ) noexcept;


  /**
   * Load and construct new certificate from DER encoded blob in \a data. On
   * failure, return null certificate and set \a error.
   */
  template <typename Ptr>
  static certificate_t from_der (const Ptr &data, std::error_code &error)
    noexcept
  {
    return from_der(
      reinterpret_cast<const uint8_t *>(data.data()),
      reinterpret_cast<const uint8_t *>(data.data()) + data.size(),
      error
    );
  }


  /**
   * Load and construct new certificate from DER encoded blob in \a data. On
   * failure, throws std::system_error
   */
  template <typename Ptr>
  static certificate_t from_der (const Ptr &data)
  {
    return from_der(data, throw_on_error("certificate::from_der"));
  }


  /**
   * Load and construct new certificate from PEM encoded blob in
   * [\a first, \a last). On failure return null certificate and set
   * \a error.
   */
  static certificate_t from_pem (const uint8_t *first, const uint8_t *last,
    std::error_code &error
  ) noexcept;


  /**
   * Load and construct new certificate from PEM encoded blob in \a data. On
   * failure, return null certificate and set \a error.
   */
  template <typename Ptr>
  static certificate_t from_pem (const Ptr &data, std::error_code &error)
    noexcept
  {
    return from_pem(
      reinterpret_cast<const uint8_t *>(data.data()),
      reinterpret_cast<const uint8_t *>(data.data()) + data.size(),
      error
    );
  }


  /**
   * Load and construct new certificate from DER encoded blob in \a data. On
   * failure, throws std::system_error
   */
  static certificate_t from_pem (const std::string &data)
  {
    return from_pem(data, throw_on_error("certificate::from_pem"));
  }


  /**
   * Convert \a this certificate into DER encoded blob in \a data and return
   * pointer to end of newly constructed blob.
   *
   * Possible errors:
   * - std::errc::bad_address: certificate is null
   * - std::errc::result_out_of_range: blob wouldn't fit into \a data
   */
  template <typename Ptr>
  uint8_t *to_der (const Ptr &data, std::error_code &error) const noexcept
  {
    return to_der(
      reinterpret_cast<uint8_t *>(data.data()),
      reinterpret_cast<uint8_t *>(data.data()) + data.size(),
      error
    );
  }


  /**
   * Convert \a this certificate into DER encoded blob in \a data and return
   * pointer to end of newly constructed blob.
   *
   * \throw std::system_error on failure.
   */
  template <typename Ptr>
  uint8_t *to_der (const Ptr &data) const
  {
    return to_der(data, throw_on_error("certificate::to_der"));
  }


  /**
   * Convert \a this certificate into DER encoded blob and return it as vector
   * of bytes.
   *
   * Possible errors:
   * - std::errc::bad_address: certificate is null
   * - std::errc::not_enough_memory: vector allocation failed
   */
  std::vector<uint8_t> to_der (std::error_code &error) const noexcept;


  /**
   * Convert \a this certificate into DER encoded blob and return it as vector
   * of bytes.
   *
   * \throws std::system_error on failure.
   */
  std::vector<uint8_t> to_der () const
  {
    std::error_code error;
    auto result = to_der(error);
    if (error)
    {
      throw_system_error(error, "certificate::to_der");
    }
    return result;
  }


  /**
   * Return true if \a this represents no certificate.
   */
  bool is_null () const noexcept
  {
    return impl_.is_null();
  }


  /**
   * Return true if \a this represents valid native certificate.
   */
  explicit operator bool () const noexcept
  {
    return !is_null();
  }


  /**
   * Returns X509 structure version (1 for V1, 3 for V3, etc) or
   * 0 if is_null() is true.
   */
  int version () const noexcept;


  /**
   * Returns absolute time since when \a this certificate is valid. On failure
   * sets \a error (cleared on success).
   */
  sal::time_t not_before (std::error_code &error) const noexcept;


  /**
   * Returns absolute time since when \a this certificate is valid.
   * \throws std::system_error on failure.
   */
  sal::time_t not_before () const
  {
    return not_before(throw_on_error("certificate::not_before"));
  }


  /**
   * Returns absolute time until \a this certificate is valid. On failure
   * sets \a error (cleared on success).
   */
  sal::time_t not_after (std::error_code &error) const noexcept;


  /**
   * Returns absolute time until \a this certificate is valid.
   * \throws std::system_error on failure.
   */
  sal::time_t not_after () const
  {
    return not_after(throw_on_error("certificate::not_after"));
  }


  /**
   * Returns true if \a this certificate is valid at absolute time \a t.
   */
  bool not_expired (sal::time_t t = now()) const
  {
    return t >= not_before() && t <= not_after();
  }


  /**
   * Returns true if \a this certificate is valid during time \a t for at
   * least duration of \a d.
   */
  template <class Rep, class Period>
  bool not_expired (const std::chrono::duration<Rep, Period> &d,
    sal::time_t t = now()) const
  {
    return t >= not_before() && t + d <= not_after();
  }


  /**
   * Returns thumbprint of \a this certificate using digest \a Algorithm.
   * On failure sets \a error (clear on success).
   */
  template <typename Algorithm>
  std::vector<uint8_t> digest (std::error_code &error) const noexcept
  {
    return apply(&digest_fn<Algorithm>, error);
  }


  /**
   * Returns thumbprint of \a this certificate using digest \a Algorithm.
   * \throws std::system_error on failure.
   */
  template <typename Algorithm>
  std::vector<uint8_t> digest () const
  {
    return digest<Algorithm>(throw_on_error("certificate::digest"));
  }


  /**
   * Returns serial number of \a this certificate.
   * On failure sets \a error (clear on success).
   */
  std::vector<uint8_t> serial_number (std::error_code &error) const noexcept;


  /**
   * Returns serial number of \a this certificate.
   * \throws std::system_error on failure.
   */
  std::vector<uint8_t> serial_number () const
  {
    return serial_number(throw_on_error("certificate::serial_number"));
  }


  /**
   * Returns authority key identifier of \a this certificate.
   * On failure sets \a error (clear on success).
   */
  std::vector<uint8_t> authority_key_identifier (std::error_code &error)
    const noexcept;


  /**
   * Returns authority key identifier of \a this certificate.
   * \throws std::system_error on failure.
   */
  std::vector<uint8_t> authority_key_identifier () const
  {
    return authority_key_identifier(
      throw_on_error("certificate::authority_key_identifier")
    );
  }


  /**
   * Returns subject key identifier of \a this certificate.
   * On failure sets \a error (clear on success).
   */
  std::vector<uint8_t> subject_key_identifier (std::error_code &error)
    const noexcept;


  /**
   * Returns subject key identifier of \a this certificate.
   * \throws std::system_error on failure.
   */
  std::vector<uint8_t> subject_key_identifier () const
  {
    return subject_key_identifier(
      throw_on_error("certificate::subject_key_identifier")
    );
  }


  /**
   * Returns true if \a this certificate is issued by \a issuer.
   * On failure sets \a error (clear on success).
   */
  bool issued_by (const certificate_t &issuer, std::error_code &error)
    const noexcept;


  /**
   * Returns true if \a this certificate is issued by \a issuer.
   * \throws std::system_error on failure.
   */
  bool issued_by (const certificate_t &issuer) const
  {
    return issued_by(issuer, throw_on_error("certificate::issued_by"));
  }


  /**
   * Returns true if \a this certificate is self signed (ie subject and issuer
   * fields are same).
   * On failure sets \a error (clear on success).
   */
  bool is_self_signed (std::error_code &error) const noexcept
  {
    return issued_by(*this, error);
  }


  /**
   * Returns true if \a this certificate is self signed (ie subject and issuer
   * fields are same).
   * \throws std::system_error on failure.
   */
  bool is_self_signed () const
  {
    return is_self_signed(throw_on_error("certificate::is_self_signed"));
  }


  /**
   * List of certificate's distinguished names as pairs of OID and textual
   * value.
   */
  using distinguished_name_t = std::vector<std::pair<oid_t, std::string>>;


  /**
   * Returns issuer's distinguished names.
   * On failure sets \a error (clear on success).
   */
  distinguished_name_t issuer (std::error_code &error) const noexcept;


  /**
   * Returns issuer's distinguished names.
   * \throws std::system_error on failure.
   */
  distinguished_name_t issuer () const
  {
    return issuer(throw_on_error("certificate::issuer"));
  }


  /**
   * Filter and return issuer's distinguished names by \a oid
   * On failure sets \a error (clear on success).
   */
  distinguished_name_t issuer (const oid_t &oid, std::error_code &error)
    const noexcept;


  /**
   * Filter and return issuer's distinguished names by \a oid
   * \throws std::system_error on failure.
   */
  distinguished_name_t issuer (const oid_t &oid) const
  {
    return issuer(oid, throw_on_error("certificate::issuer"));
  }


  /**
   * Returns subject's distinguished names.
   * On failure sets \a error (clear on success).
   */
  distinguished_name_t subject (std::error_code &error) const noexcept;


  /**
   * Returns subject's distinguished names.
   * \throws std::system_error on failure.
   */
  distinguished_name_t subject () const
  {
    return subject(throw_on_error("certificate::subject"));
  }


  /**
   * Filter and return subject's distinguished names by \a oid
   * On failure sets \a error (clear on success).
   */
  distinguished_name_t subject (const oid_t &oid, std::error_code &error)
    const noexcept;


  /**
   * Filter and return subject's distinguished names by \a oid
   * \throws std::system_error on failure.
   */
  distinguished_name_t subject (const oid_t &oid) const
  {
    return subject(oid, throw_on_error("certificate::subject"));
  }


  /**
   * Certificate alternative name types.
   */
  enum class alt_name
  {
    dns,        ///< DNS name
    ip,         ///< IP address
    uri,        ///< URI
    email,      ///< Email address
  };


  /**
   * Returns list of issuer's alternative names as pairs of types and
   * corresponding textual values.
   * On failure sets \a error (clear on success).
   */
  std::vector<std::pair<alt_name, std::string>> issuer_alt_names (
    std::error_code &error
  ) const noexcept;


  /**
   * Returns list of issuer's alternative names as pairs of types and
   * corresponding textual values.
   * \throws std::system_error on failure.
   */
  std::vector<std::pair<alt_name, std::string>> issuer_alt_names () const
  {
    return issuer_alt_names(throw_on_error("certificate::issuer_alt_names"));
  }


  /**
   * Returns list of subject's alternative names as pairs of types and
   * corresponding textual values.
   * On failure sets \a error (clear on success).
   */
  std::vector<std::pair<alt_name, std::string>> subject_alt_names (
    std::error_code &error
  ) const noexcept;


  /**
   * Returns list of subject's alternative names as pairs of types and
   * corresponding textual values.
   * \throws std::system_error on failure.
   */
  std::vector<std::pair<alt_name, std::string>> subject_alt_names () const
  {
    return subject_alt_names(throw_on_error("certificate::subject_alt_names"));
  }


  /**
   * Returns certificate's public key.
   * On failure sets error (clear on success).
   */
  public_key_t public_key (std::error_code &error) const noexcept;


  /**
   * Returns certificate's public key.
   * \throws std::system_error on failure.
   */
  public_key_t public_key () const
  {
    return public_key(throw_on_error("certificate::public_key"));
  }


  /**
   * Helper structure to create textual representation of list of
   * distinguished names.
   *
   * \note All attributes are references ie original variables must be kept
   * live until inserter methods have finished.
   */
  struct distinguished_name_format_t
  {
    /// Reference to distinguished names for formatting
    const distinguished_name_t &rdn;

    /// String to use as assignment
    const std::string &assign;

    /// Separator between distinguished names
    const std::string &separator;

    /// Ctor
    distinguished_name_format_t (const distinguished_name_t &rdn,
        const std::string &assign,
        const std::string &separator) noexcept
      : rdn(rdn)
      , assign(assign)
      , separator(separator)
    {}
  };


  /**
   * Convenience method to create distinguished names list formatter.
   */
  static distinguished_name_format_t format (
    const distinguished_name_t &rdn,
    const std::string &assign = "=",
    const std::string &separator = "; ") noexcept
  {
    return distinguished_name_format_t{rdn, assign, separator};
  }


private:

  __bits::certificate_t impl_{};

  certificate_t (__bits::certificate_t &&that)
    : impl_(std::move(that))
  {}

  uint8_t *to_der (uint8_t *first, uint8_t *last, std::error_code &error)
    const noexcept;

  template <typename Algorithm>
  static std::vector<uint8_t> digest_fn (const uint8_t *data, size_t size)
  {
    std::vector<uint8_t> digest(hash_t<Algorithm>::digest_size);
    hash_t<Algorithm>::one_shot(
      data, data + size,
      digest.begin(), digest.end()
    );
    return digest;
  }

  std::vector<uint8_t> apply (
    std::vector<uint8_t>(*fn)(const uint8_t *, size_t),
    std::error_code &error
  ) const noexcept;
};


/**
 * \see certificate_t::import_pkcs12
 */
template <typename DataPtr>
inline certificate_t import_pkcs12 (const DataPtr &pkcs12,
  const std::string &passphrase,
  private_key_t &private_key,
  std::vector<certificate_t> &chain,
  std::error_code &error) noexcept
{
  return certificate_t::import_pkcs12(
    reinterpret_cast<const uint8_t *>(pkcs12.data()),
    reinterpret_cast<const uint8_t *>(pkcs12.data()) + pkcs12.size(),
    passphrase,
    &private_key,
    &chain,
    error
  );
}


/**
 * \see certificate_t::import_pkcs12
 */
template <typename DataPtr>
inline certificate_t import_pkcs12 (const DataPtr &pkcs12,
  const std::string &passphrase,
  private_key_t &private_key,
  std::vector<certificate_t> &chain)
{
  return import_pkcs12(pkcs12, passphrase, private_key, chain,
    throw_on_error("import_pkcs12")
  );
}


/**
 * \see certificate_t::import_pkcs12
 */
template <typename DataPtr>
inline certificate_t import_pkcs12 (const DataPtr &pkcs12,
  const std::string &passphrase,
  private_key_t &private_key,
  std::error_code &error) noexcept
{
  return certificate_t::import_pkcs12(
    reinterpret_cast<const uint8_t *>(pkcs12.data()),
    reinterpret_cast<const uint8_t *>(pkcs12.data()) + pkcs12.size(),
    passphrase,
    &private_key,
    nullptr,
    error
  );
}


/**
 * \see certificate_t::import_pkcs12
 */
template <typename DataPtr>
inline certificate_t import_pkcs12 (const DataPtr &pkcs12,
  const std::string &passphrase,
  private_key_t &private_key)
{
  return import_pkcs12(pkcs12, passphrase, private_key,
    throw_on_error("import_pkcs12")
  );
}


/**
 * \see certificate_t::import_pkcs12
 */
template <typename DataPtr>
inline certificate_t import_pkcs12 (const DataPtr &pkcs12,
  const std::string &passphrase,
  std::vector<certificate_t> &chain,
  std::error_code &error) noexcept
{
  return certificate_t::import_pkcs12(
    reinterpret_cast<const uint8_t *>(pkcs12.data()),
    reinterpret_cast<const uint8_t *>(pkcs12.data()) + pkcs12.size(),
    passphrase,
    nullptr,
    &chain,
    error
  );
}


/**
 * \see certificate_t::import_pkcs12
 */
template <typename DataPtr>
inline certificate_t import_pkcs12 (const DataPtr &pkcs12,
  const std::string &passphrase,
  std::vector<certificate_t> &chain)
{
  return import_pkcs12(pkcs12, passphrase, chain,
    throw_on_error("import_pkcs12")
  );
}


/**
 * \see certificate_t::import_pkcs12
 */
template <typename DataPtr>
inline certificate_t import_pkcs12 (const DataPtr &pkcs12,
  const std::string &passphrase,
  std::error_code &error) noexcept
{
  return certificate_t::import_pkcs12(
    reinterpret_cast<const uint8_t *>(pkcs12.data()),
    reinterpret_cast<const uint8_t *>(pkcs12.data()) + pkcs12.size(),
    passphrase,
    nullptr,
    nullptr,
    error
  );
}


/**
 * \see certificate_t::import_pkcs12
 */
template <typename DataPtr>
inline certificate_t import_pkcs12 (const DataPtr &pkcs12,
  const std::string &passphrase)
{
  return import_pkcs12(pkcs12, passphrase, throw_on_error("import_pkcs12"));
}


/**
 * Format certificate's distinguished names list in \a rdn into memory
 * \a writer.
 */
memory_writer_t &operator<< (memory_writer_t &writer,
  const certificate_t::distinguished_name_format_t &rdn
);


/**
 * Format certificate's subject distinguished names list in \a rdn into memory
 * \a writer.
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const certificate_t &certificate)
{
  return (writer << certificate_t::format(certificate.subject()));
}


/**
 * Format certificate's distinguished names list in \a rdn into stream \a os.
 */
inline std::ostream &operator<< (std::ostream &os,
  const certificate_t::distinguished_name_format_t &rdn)
{
  char_array_t<1024> buf;
  return (os << ((buf << rdn) ? buf.c_str() : "<...>"));
}


/**
 * Format certificate's subject distinguished names list in \a rdn into stream
 * \a os
 */
inline std::ostream &operator<< (std::ostream &os,
  const certificate_t &certificate)
{
  return (os << certificate_t::format(certificate.subject()));
}


} // namespace crypto


__sal_end
