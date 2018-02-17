#pragma once

/**
 * \file sal/crypto/pipe_options.hpp
 * Secure encrypted channel options.
 */


#include <sal/config.hpp>
#include <sal/crypto/certificate.hpp>
#include <string>


__sal_begin


namespace crypto {


/**
 * \defgroup Pipe options
 * \{
 */


template <typename Option>
struct pipe_option_t
{};


struct peer_name_t
  : public pipe_option_t<peer_name_t>
{
  std::string peer_name;

  peer_name_t (std::string peer_name)
    : peer_name(peer_name)
  {}
};


inline peer_name_t peer_name (std::string name)
{
  return {name};
}


/// \}


/**
 * \defgroup Pipe factory options
 */

template <typename Option>
struct pipe_factory_option_t
{};


struct mutual_auth_t: pipe_factory_option_t<mutual_auth_t>
{};


inline constexpr const mutual_auth_t mutual_auth{};


struct with_certificate_t
  : public pipe_factory_option_t<with_certificate_t>
{
  certificate_t certificate;

  with_certificate_t (certificate_t certificate) noexcept
    : certificate(certificate)
  {}
};


inline with_certificate_t with_certificate (certificate_t certificate)
  noexcept
{
  return {certificate};
}


struct with_private_key_t
  : public pipe_factory_option_t<with_private_key_t>
{
  const private_key_t * const private_key;

  with_private_key_t (const private_key_t *private_key) noexcept
    : private_key(private_key)
  {}
};


inline with_private_key_t with_private_key (const private_key_t *private_key)
  noexcept
{
  return {private_key};
}


template <typename Check>
struct manual_certificate_check_t
  : public pipe_factory_option_t<manual_certificate_check_t<Check>>
{
  Check check;

  manual_certificate_check_t (Check check) noexcept
    : check(check)
  {}
};


template <typename Check>
inline manual_certificate_check_t<Check> manual_certificate_check (Check check)
  noexcept
{
  return {check};
}


inline const auto no_certificate_check = manual_certificate_check(
  [](const certificate_t &) noexcept
  {
    return true;
  }
);


/// \}


} // namespace crypto


__sal_end
