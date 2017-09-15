#include <sal/crypto/key.hpp>
#include <sal/common.test.hpp>


namespace {

using public_key_t = sal::crypto::public_key_t;


struct crypto_key
  : public sal_test::fixture
{};


TEST_F(crypto_key, public_key_ctor) //{{{1
{
  public_key_t key;
  EXPECT_TRUE(key.is_null());
}


//}}}1


} // namespace
