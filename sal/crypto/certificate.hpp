#pragma once

/**
 * \file sal/crypto/certificate.hpp
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/certificate.hpp>
#include <sal/crypto/error.hpp>
#include <sal/crypto/oid.hpp>
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
  template <typename Ptr>
  static certificate_t from_der (const Ptr &data, std::error_code &error)
    noexcept
  {
    return certificate_t(
      reinterpret_cast<const uint8_t *>(data.data()),
      reinterpret_cast<const uint8_t *>(data.data() + data.size()),
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
  static certificate_t from_pem (const std::string &data,
    std::error_code &error
  ) noexcept;


  /**
   */
  static certificate_t from_pem (const std::string &data)
  {
    return from_pem(data, throw_on_error("certificate::from_pem"));
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


private:

  __bits::certificate_t impl_{};

  certificate_t (const uint8_t *first, const uint8_t *last,
    std::error_code &error
  ) noexcept;
};


} // namespace crypto


__sal_end
