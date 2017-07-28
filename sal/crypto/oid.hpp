#pragma once

/**
 * \file sal/crypto/oid.hpp
 * Object identifiers for X509 certificates
 */


#include <sal/config.hpp>
#include <string>


__sal_begin


namespace crypto {


using oid_t = std::string;


namespace oid {

extern const std::string
  collective_state_or_province_name,
  collective_street_address,
  common_name,
  country_name,
  description,
  given_name,
  locality_name,
  organization_name,
  organizational_unit_name,
  serial_number,
  state_or_province_name,
  street_address,
  surname,
  title;

} // namespace oid


} // namespace crypto


__sal_end

