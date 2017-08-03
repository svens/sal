#pragma once

/**
 * \file sal/crypto/certificate.hpp
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/certificate.hpp>
#include <sal/crypto/error.hpp>
#include <sal/crypto/oid.hpp>
#include <vector>


__sal_begin


namespace crypto {


class certificate_t
{
public:

  certificate_t () = default;


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
  std::vector<uint8_t> serial_number (std::error_code &error) const noexcept;


  /**
   */
  std::vector<uint8_t> serial_number () const
  {
    return serial_number(throw_on_error("certificate::serial_number"));
  }


  /**
   */
  // bool is_authority () const noexcept;


  /**
   */
  // bool is_self_signed () const noexcept;


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
