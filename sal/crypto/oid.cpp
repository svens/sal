#include <sal/crypto/oid.hpp>


__sal_begin


namespace crypto {


const std::string &alias_or_oid (const oid_t &oid) noexcept
{
  static const std::pair<oid_t, std::string> oid_alias[] =
  {
    { oid::common_name, "CN" },
    { oid::country_name, "C" },
    { oid::locality_name, "L" },
    { oid::organization_name, "O" },
    { oid::organizational_unit_name, "OU" },
    { oid::state_or_province_name, "ST" },
    { oid::street_address, "STREET" },
  };

  for (const auto &alias: oid_alias)
  {
    if (oid == alias.first)
    {
      return alias.second;
    }
  }

  return oid;
}


} // namespace crypto


__sal_end
