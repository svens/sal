#include <sal/crypto/key.hpp>
#include <sal/crypto/certificate.hpp>
#include <sal/crypto/common.test.hpp>


namespace {


using namespace sal_test;

using public_key_t = sal::crypto::public_key_t;
using private_key_t = sal::crypto::private_key_t;


struct crypto_key
  : public sal_test::fixture
{
  public_key_t public_key;
  private_key_t private_key;

  void SetUp ()
  {
    public_key = sal::crypto::import_pkcs12(
      to_der(cert::pkcs12),
      "TestPassword",
      private_key
    ).public_key();

    ASSERT_TRUE(!public_key.is_null());
    ASSERT_TRUE(!private_key.is_null());
  }
};


TEST_F(crypto_key, public_key_ctor) //{{{1
{
  public_key_t key;
  EXPECT_TRUE(key.is_null());
}


TEST_F(crypto_key, public_key_ctor_move) //{{{1
{
  auto key = std::move(public_key);
  ASSERT_FALSE(key.is_null());
  EXPECT_EQ(sal::crypto::key_type::rsa, key.type());
  EXPECT_EQ(256U, key.block_size());

  EXPECT_TRUE(public_key.is_null());
}


TEST_F(crypto_key, public_key_assign_move) //{{{1
{
  public_key_t key;
  EXPECT_TRUE(key.is_null());

  key = std::move(public_key);
  ASSERT_FALSE(key.is_null());
  EXPECT_EQ(sal::crypto::key_type::rsa, key.type());
  EXPECT_EQ(256U, key.block_size());

  EXPECT_TRUE(public_key.is_null());
}


TEST_F(crypto_key, public_key_is_null) //{{{1
{
  public_key_t key;
  EXPECT_TRUE(key.is_null());
  EXPECT_TRUE(!key);
}


TEST_F(crypto_key, public_key_is_not_null) //{{{1
{
  EXPECT_FALSE(public_key.is_null());
  EXPECT_FALSE(!public_key);
}


TEST_F(crypto_key, public_key_swap) //{{{1
{
  EXPECT_EQ(sal::crypto::key_type::rsa, public_key.type());
  EXPECT_EQ(256U, public_key.block_size());

  public_key_t key;
  EXPECT_TRUE(key.is_null());

  public_key.swap(key);
  EXPECT_TRUE(public_key.is_null());

  ASSERT_FALSE(key.is_null());
  EXPECT_EQ(sal::crypto::key_type::rsa, key.type());
  EXPECT_EQ(256U, key.block_size());
}


TEST_F(crypto_key, public_key_properties) //{{{1
{
  ASSERT_FALSE(public_key.is_null());
  EXPECT_EQ(sal::crypto::key_type::rsa, public_key.type());
  EXPECT_EQ(256U, public_key.block_size());
}

//}}}1

TEST_F(crypto_key, private_key_ctor) //{{{1
{
  private_key_t key;
  EXPECT_TRUE(key.is_null());
}


TEST_F(crypto_key, private_key_ctor_move) //{{{1
{
  auto key = std::move(private_key);
  ASSERT_FALSE(key.is_null());
  EXPECT_EQ(sal::crypto::key_type::rsa, key.type());
  EXPECT_EQ(256U, key.block_size());

  EXPECT_TRUE(private_key.is_null());
}


TEST_F(crypto_key, private_key_assign_move) //{{{1
{
  private_key_t key;
  EXPECT_TRUE(key.is_null());

  key = std::move(private_key);
  ASSERT_FALSE(key.is_null());
  EXPECT_EQ(sal::crypto::key_type::rsa, key.type());
  EXPECT_EQ(256U, key.block_size());

  EXPECT_TRUE(private_key.is_null());
}


TEST_F(crypto_key, private_key_is_null) //{{{1
{
  private_key_t key;
  EXPECT_TRUE(key.is_null());
  EXPECT_TRUE(!key);
}


TEST_F(crypto_key, private_key_is_not_null) //{{{1
{
  EXPECT_FALSE(private_key.is_null());
  EXPECT_FALSE(!private_key);
}


TEST_F(crypto_key, private_key_swap) //{{{1
{
  EXPECT_EQ(sal::crypto::key_type::rsa, private_key.type());
  EXPECT_EQ(256U, private_key.block_size());

  private_key_t key;
  EXPECT_TRUE(key.is_null());

  private_key.swap(key);
  EXPECT_TRUE(private_key.is_null());

  ASSERT_FALSE(key.is_null());
  EXPECT_EQ(sal::crypto::key_type::rsa, key.type());
  EXPECT_EQ(256U, key.block_size());
}


TEST_F(crypto_key, private_key_properties) //{{{1
{
  ASSERT_FALSE(private_key.is_null());
  EXPECT_EQ(sal::crypto::key_type::rsa, private_key.type());
  EXPECT_EQ(256U, private_key.block_size());
}

//}}}1


} // namespace
