#pragma once

/**
 * \file sal/crypto/oid.hpp
 * Object identifiers for X509 certificates
 */


#include <sal/config.hpp>
#include <string>


__sal_begin


namespace crypto {


/**
 * Textual OID representation
 */
using oid_t = std::string;


namespace oid {

extern const oid_t
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.8.1
  collective_state_or_province_name,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.9.1
  collective_street_address,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.3
  common_name,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.6
  description,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.13
  country_name,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.42
  given_name,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.7
  locality_name,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.10
  organization_name,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.11
  organizational_unit_name,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.5
  serial_number,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.8
  state_or_province_name,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.9
  street_address,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.4
  surname,
  /// http://www.oid-info.com/cgi-bin/display?action=display&oid=2.5.4.12
  title;

} // namespace oid


/**
 * Return alias for \a oid. Supported OID aliases are listed in RFC 1779 2.3.
 * If there is no alias for \a oid, it is returned itself.
 */
const std::string &alias_or_oid (const oid_t &oid) noexcept;


} // namespace crypto


__sal_end
