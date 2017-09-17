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

TEST_F(crypto_key, sign_and_verify_signature) //{{{1
{
  std::vector<uint8_t> data(case_name.begin(), case_name.end());
  uint8_t signature[1024];

  // sign
  std::error_code error;
  auto end = private_key.sign(sal::crypto::sign_digest_type::sha1,
    data.data(), data.data() + data.size(),
    signature, signature + sizeof(signature),
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_EQ(private_key.block_size(), static_cast<size_t>(end - signature));

  // verify digest signature
  auto is_valid = public_key.verify_signature(
    sal::crypto::sign_digest_type::sha1,
    data.data(), data.data() + data.size(),
    signature, end,
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_TRUE(is_valid);
}


TEST_F(crypto_key, sign_with_null_private_key) //{{{1
{
  private_key_t key;
  std::vector<uint8_t> data(case_name.begin(), case_name.end());
  uint8_t signature[1024];

  // sign
  std::error_code error;
  (void)key.sign(sal::crypto::sign_digest_type::sha1,
    data.data(), data.data() + data.size(),
    signature, signature + sizeof(signature),
    error
  );
  EXPECT_EQ(std::errc::bad_address, error);
}


TEST_F(crypto_key, sign_with_invalid_digest_type) //{{{1
{
  std::vector<uint8_t> data(case_name.begin(), case_name.end());
  uint8_t signature[1024];

  // sign
  auto digest_type = static_cast<sal::crypto::sign_digest_type>(99);
  std::error_code error;
  (void)private_key.sign(digest_type,
    data.data(), data.data() + data.size(),
    signature, signature + sizeof(signature),
    error
  );
  EXPECT_EQ(std::errc::invalid_argument, error);
}


TEST_F(crypto_key, sign_and_verify_signature_with_null_public_key) //{{{1
{
  std::vector<uint8_t> data(case_name.begin(), case_name.end());
  uint8_t signature[1024];

  // sign
  std::error_code error;
  auto end = private_key.sign(sal::crypto::sign_digest_type::sha1,
    data.data(), data.data() + data.size(),
    signature, signature + sizeof(signature),
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_EQ(private_key.block_size(), static_cast<size_t>(end - signature));

  // verify digest signature
  public_key_t key;
  (void)key.verify_signature(
    sal::crypto::sign_digest_type::sha1,
    data.data(), data.data() + data.size(),
    signature, end,
    error
  );
  EXPECT_EQ(std::errc::bad_address, error);
}


TEST_F(crypto_key, sign_and_verify_signature_invalid_signature) //{{{1
{
  std::vector<uint8_t> data(case_name.begin(), case_name.end());
  uint8_t signature[1024];

  // sign
  std::error_code error;
  auto end = private_key.sign(sal::crypto::sign_digest_type::sha1,
    data.data(), data.data() + data.size(),
    signature, signature + sizeof(signature),
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_EQ(private_key.block_size(), static_cast<size_t>(end - signature));

  // invalidate
  signature[0] ^= 1;

  // verify digest signature
  auto is_valid = public_key.verify_signature(
    sal::crypto::sign_digest_type::sha1,
    data.data(), data.data() + data.size(),
    signature, end,
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_FALSE(is_valid);
}


TEST_F(crypto_key, verify_signature_with_different_digest_type)
{
  std::vector<uint8_t> data(case_name.begin(), case_name.end());
  uint8_t signature[1024];

  // sign
  std::error_code error;
  auto end = private_key.sign(sal::crypto::sign_digest_type::sha1,
    data.data(), data.data() + data.size(),
    signature, signature + sizeof(signature),
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_EQ(private_key.block_size(), static_cast<size_t>(end - signature));

  // verify digest signature
  auto digest_type = sal::crypto::sign_digest_type::sha512;
  auto is_valid = public_key.verify_signature(digest_type,
    data.data(), data.data() + data.size(),
    signature, end,
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_FALSE(is_valid);
}


TEST_F(crypto_key, verify_signature_with_invalid_digest)
{
  std::vector<uint8_t> data(case_name.begin(), case_name.end());
  uint8_t signature[1024];

  // sign
  std::error_code error;
  auto end = private_key.sign(sal::crypto::sign_digest_type::sha1,
    data.data(), data.data() + data.size(),
    signature, signature + sizeof(signature),
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_EQ(private_key.block_size(), static_cast<size_t>(end - signature));

  // verify digest signature
  auto digest_type = static_cast<sal::crypto::sign_digest_type>(99);
  (void)public_key.verify_signature(digest_type,
    data.data(), data.data() + data.size(),
    signature, end,
    error
  );
  EXPECT_EQ(std::errc::invalid_argument, error);
}


//}}}1


} // namespace
