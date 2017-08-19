#pragma once

/**
 * \file sal/crypto/certificate.hpp
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/x509.hpp>
#include <sal/crypto/hash.hpp>
#include <sal/crypto/key.hpp>
#include <sal/crypto/oid.hpp>
#include <sal/buf_ptr.hpp>
#include <sal/time.hpp>
#include <vector>


__sal_begin


namespace crypto {


class certificate_t
{
public:

  certificate_t () = default;
  certificate_t (const certificate_t &) = default;
  certificate_t (certificate_t &&) = default;
  certificate_t &operator= (const certificate_t &) = default;
  certificate_t &operator= (certificate_t &&) = default;


  /**
   */
  void swap (certificate_t &that) noexcept
  {
    impl_.swap(that.impl_);
  }


  /**
   */
  bool operator== (const certificate_t &that) const noexcept;


  /**
   */
  bool operator!= (const certificate_t &that) const noexcept
  {
    return !operator==(that);
  }


  /**
   */
  static certificate_t import_pkcs12 (
    const uint8_t *first, const uint8_t *last,
    const std::string &passphrase,
    private_key_t *private_key,
    std::vector<certificate_t> *chain,
    std::error_code &error
  ) noexcept;


  /**
   */
  static certificate_t from_der (const uint8_t *first, const uint8_t *last,
    std::error_code &error
  ) noexcept;


  /**
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
   */
  template <typename Ptr>
  static certificate_t from_der (const Ptr &data)
  {
    return from_der(data, throw_on_error("certificate::from_der"));
  }


  /**
   */
  static certificate_t from_pem (const uint8_t *first, const uint8_t *last,
    std::error_code &error
  ) noexcept;


  /**
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
   */
  static certificate_t from_pem (const std::string &data)
  {
    return from_pem(data, throw_on_error("certificate::from_pem"));
  }


  /**
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
   */
  template <typename Ptr>
  uint8_t *to_der (const Ptr &data) const
  {
    return to_der(data, throw_on_error("certificate::to_der"));
  }


  /**
   */
  std::vector<uint8_t> to_der (std::error_code &error) const noexcept;


  /**
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


  /**
   */
  int version () const noexcept;


  /**
   */
  sal::time_t not_before (std::error_code &error) const noexcept;


  /**
   */
  sal::time_t not_before () const
  {
    return not_before(throw_on_error("certificate::not_before"));
  }


  /**
   */
  sal::time_t not_after (std::error_code &error) const noexcept;


  /**
   */
  sal::time_t not_after () const
  {
    return not_after(throw_on_error("certificate::not_after"));
  }


  /**
   */
  bool not_expired (sal::time_t t = now()) const
  {
    return t >= not_before() && t <= not_after();
  }


  /**
   */
  template <class Rep, class Period>
  bool not_expired (const std::chrono::duration<Rep, Period> &d,
    sal::time_t t = now()) const
  {
    return t >= not_before() && t + d <= not_after();
  }


  /**
   */
  template <typename Algorithm>
  std::vector<uint8_t> digest (std::error_code &error) const noexcept
  {
    return apply(&digest_fn<Algorithm>, error);
  }


  /**
   */
  template <typename Algorithm>
  std::vector<uint8_t> digest () const
  {
    return digest<Algorithm>(throw_on_error("certificate::digest"));
  }


  /**
   */
  std::vector<uint8_t> serial_number (std::error_code &error) const noexcept;


  /**
   */
  std::vector<uint8_t> serial_number () const
  {
    return serial_number(throw_on_error("certificate::serial_number"));
  }


  /**
   */
  std::vector<uint8_t> authority_key_identifier (std::error_code &error)
    const noexcept;


  /**
   */
  std::vector<uint8_t> authority_key_identifier () const
  {
    return authority_key_identifier(
      throw_on_error("certificate::authority_key_identifier")
    );
  }


  /**
   */
  std::vector<uint8_t> subject_key_identifier (std::error_code &error)
    const noexcept;


  /**
   */
  std::vector<uint8_t> subject_key_identifier () const
  {
    return subject_key_identifier(
      throw_on_error("certificate::subject_key_identifier")
    );
  }


  /**
   */
  bool issued_by (const certificate_t &issuer, std::error_code &error)
    const noexcept;


  /**
   */
  bool issued_by (const certificate_t &issuer) const
  {
    return issued_by(issuer, throw_on_error("certificate::issued_by"));
  }


  /**
   */
  bool is_self_signed (std::error_code &error) const noexcept
  {
    return issued_by(*this, error);
  }


  /**
   */
  bool is_self_signed () const
  {
    return is_self_signed(throw_on_error("certificate::is_self_signed"));
  }


  /**
   */
  using distinguished_name_t = std::vector<std::pair<oid_t, std::string>>;


  /**
   */
  distinguished_name_t issuer (std::error_code &error) const noexcept;


  /**
   */
  distinguished_name_t issuer () const
  {
    return issuer(throw_on_error("certificate::issuer"));
  }


  /**
   */
  distinguished_name_t issuer (const oid_t &oid, std::error_code &error)
    const noexcept;


  /**
   */
  distinguished_name_t issuer (const oid_t &oid) const
  {
    return issuer(oid, throw_on_error("certificate::issuer"));
  }


  /**
   */
  distinguished_name_t subject (std::error_code &error) const noexcept;


  /**
   */
  distinguished_name_t subject () const
  {
    return subject(throw_on_error("certificate::subject"));
  }


  /**
   */
  distinguished_name_t subject (const oid_t &oid, std::error_code &error)
    const noexcept;


  /**
   */
  distinguished_name_t subject (const oid_t &oid) const
  {
    return subject(oid, throw_on_error("certificate::subject"));
  }


  /**
   */
  enum class alt_name
  {
    dns,
    ip,
    uri,
    email,
  };


  /**
   */
  std::vector<std::pair<alt_name, std::string>> issuer_alt_names (
    std::error_code &error
  ) const noexcept;


  /**
   */
  std::vector<std::pair<alt_name, std::string>> issuer_alt_names () const
  {
    return issuer_alt_names(throw_on_error("certificate::issuer_alt_names"));
  }


  /**
   */
  std::vector<std::pair<alt_name, std::string>> subject_alt_names (
    std::error_code &error
  ) const noexcept;


  /**
   */
  std::vector<std::pair<alt_name, std::string>> subject_alt_names () const
  {
    return subject_alt_names(throw_on_error("certificate::subject_alt_names"));
  }


  /**
   */
  public_key_t public_key (std::error_code &error) const noexcept;


  /**
   */
  public_key_t public_key () const
  {
    return public_key(throw_on_error("certificate::public_key"));
  }


private:

  __bits::certificate_t impl_{};

  certificate_t (__bits::certificate_t &&that)
    : impl_(std::move(that))
  {}

  uint8_t *to_der (uint8_t *first, uint8_t *last, std::error_code &error)
    const noexcept;

  template <typename Algorithm>
  static std::vector<uint8_t> digest_fn (const void *data, size_t size)
  {
    return hash_t<Algorithm>::one_shot(make_buf(data, size));
  }

  std::vector<uint8_t> apply (
    std::vector<uint8_t>(*fn)(const void *, size_t),
    std::error_code &error
  ) const noexcept;
};


/**
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
 */
template <typename DataPtr>
inline certificate_t import_pkcs12 (const DataPtr &pkcs12,
  const std::string &passphrase)
{
  return import_pkcs12(pkcs12, passphrase, throw_on_error("import_pkcs12"));
}


} // namespace crypto


__sal_end
