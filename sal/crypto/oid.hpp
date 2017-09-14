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


/**
 * Return alias for \a oid. Supported OID aliases are listed in RFC 1779 2.3.
 * If there is no alias for \a oid, it is returned itself.
 */
const std::string &alias_or_oid (const oid_t &oid) noexcept;


} // namespace crypto


__sal_end
