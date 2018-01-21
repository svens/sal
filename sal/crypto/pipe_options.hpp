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

  with_certificate_t (const certificate_t &certificate) noexcept
    : certificate(certificate)
  {}
};


inline with_certificate_t with_certificate (const certificate_t &certificate)
  noexcept
{
  return {certificate};
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
