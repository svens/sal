#include <sal/crypto/oid.hpp>
#include <sal/common.test.hpp>


namespace {

using oid_t = sal::crypto::oid_t;
namespace oid = sal::crypto::oid;


struct crypto_oid
  : public sal_test::fixture
{};


TEST_F(crypto_oid, alias_or_oid) //{{{1
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

  for (auto alias: oid_alias)
  {
    EXPECT_EQ(alias.second, sal::crypto::alias_or_oid(alias.first));
  }
}


TEST_F(crypto_oid, alias_or_oid_not_found) //{{{1
{
  EXPECT_EQ("1.1", sal::crypto::alias_or_oid("1.1"));
}


//}}}1


} // namespace
