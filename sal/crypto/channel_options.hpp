#pragma once

/**
 * \file sal/crypto/channel_options.hpp
 * Secure encrypted channel options.
 */


#include <sal/config.hpp>
#include <sal/crypto/certificate.hpp>
#include <string>


__sal_begin


namespace crypto {


/**
 * \defgroup Channel options
 * \{
 */


template <typename Option>
struct channel_option_t
{};


struct mutual_auth_t
  : public channel_option_t<mutual_auth_t>
{
  const bool value;

  constexpr mutual_auth_t (bool required) noexcept
    : value(required)
  { }
};


inline constexpr const mutual_auth_t mutual_auth{true};
inline constexpr const mutual_auth_t no_mutual_auth{false};


struct peer_name_t
  : public channel_option_t<peer_name_t>
{
  const std::string value;

  peer_name_t (std::string peer_name)
    : value(peer_name)
  {}
};


inline peer_name_t peer_name (std::string name)
{
  return {name};
}


/// \}


/**
 * \defgroup Channel factory options
 */

template <typename Option>
struct channel_factory_option_t
{};


struct with_certificate_t
  : public channel_factory_option_t<with_certificate_t>
{
  certificate_t value;

  with_certificate_t (certificate_t certificate) noexcept
    : value(certificate)
  {}
};


inline with_certificate_t with_certificate (certificate_t certificate)
  noexcept
{
  return {certificate};
}


struct with_private_key_t
  : public channel_factory_option_t<with_private_key_t>
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
  : public channel_factory_option_t<manual_certificate_check_t<Check>>
{
  Check value;

  manual_certificate_check_t (Check check) noexcept
    : value(check)
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
