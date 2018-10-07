#include <sal/crypto/key.hpp>
#include <sal/crypto/certificate.hpp>
#include <sal/crypto/common.test.hpp>


namespace {


using namespace sal_test;

using public_key_t = sal::crypto::public_key_t;
using private_key_t = sal::crypto::private_key_t;


auto import ()
{
  private_key_t private_key;
  auto chain = sal::crypto::import_pkcs12(
    to_der(cert::pkcs12),
    "TestPassword",
    &private_key
  );

  return std::make_pair(
    chain[0].public_key(),
    std::move(private_key)
  );
}


struct crypto_key
  : public sal_test::fixture
{
  public_key_t public_key{};
  private_key_t private_key{};

  void SetUp ()
  {
    std::tie(public_key, private_key) = import();
    ASSERT_TRUE(!public_key.is_null());
    ASSERT_TRUE(!private_key.is_null());
  }
};


template <typename Digest>
struct crypto_key_with_digest
  : public sal_test::with_type<Digest>
{
  public_key_t public_key{};
  private_key_t private_key{};

  void SetUp ()
  {
    std::tie(public_key, private_key) = import();
    ASSERT_TRUE(!public_key.is_null());
    ASSERT_TRUE(!private_key.is_null());
  }
};


using digest_types = ::testing::Types<
  sal::crypto::sha1,
  sal::crypto::sha256,
  sal::crypto::sha384,
  sal::crypto::sha512
>;
TYPED_TEST_CASE(crypto_key_with_digest, digest_types, );


TEST_F(crypto_key, public_key_ctor) //{{{1
{
  public_key_t key;
  EXPECT_TRUE(key.is_null());
}


TEST_F(crypto_key, public_key_ctor_move) //{{{1
{
  auto key = std::move(public_key);
  ASSERT_FALSE(key.is_null());
  EXPECT_EQ(sal::crypto::key_algorithm::rsa, key.algorithm());
  EXPECT_EQ(256U, key.block_size());

  EXPECT_TRUE(public_key.is_null());
}


TEST_F(crypto_key, public_key_assign_move) //{{{1
{
  public_key_t key;
  EXPECT_TRUE(key.is_null());

  key = std::move(public_key);
  ASSERT_FALSE(key.is_null());
  EXPECT_EQ(sal::crypto::key_algorithm::rsa, key.algorithm());
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
  EXPECT_EQ(sal::crypto::key_algorithm::rsa, public_key.algorithm());
  EXPECT_EQ(256U, public_key.block_size());

  public_key_t key;
  EXPECT_TRUE(key.is_null());

  public_key.swap(key);
  EXPECT_TRUE(public_key.is_null());

  ASSERT_FALSE(key.is_null());
  EXPECT_EQ(sal::crypto::key_algorithm::rsa, key.algorithm());
  EXPECT_EQ(256U, key.block_size());
}


TEST_F(crypto_key, public_key_properties) //{{{1
{
  ASSERT_FALSE(public_key.is_null());
  EXPECT_EQ(sal::crypto::key_algorithm::rsa, public_key.algorithm());
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
  EXPECT_EQ(sal::crypto::key_algorithm::rsa, key.algorithm());
  EXPECT_EQ(256U, key.block_size());

  EXPECT_TRUE(private_key.is_null());
}


TEST_F(crypto_key, private_key_assign_move) //{{{1
{
  private_key_t key;
  EXPECT_TRUE(key.is_null());

  key = std::move(private_key);
  ASSERT_FALSE(key.is_null());
  EXPECT_EQ(sal::crypto::key_algorithm::rsa, key.algorithm());
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
  EXPECT_EQ(sal::crypto::key_algorithm::rsa, private_key.algorithm());
  EXPECT_EQ(256U, private_key.block_size());

  private_key_t key;
  EXPECT_TRUE(key.is_null());

  private_key.swap(key);
  EXPECT_TRUE(private_key.is_null());

  ASSERT_FALSE(key.is_null());
  EXPECT_EQ(sal::crypto::key_algorithm::rsa, key.algorithm());
  EXPECT_EQ(256U, key.block_size());
}


TEST_F(crypto_key, private_key_properties) //{{{1
{
  ASSERT_FALSE(private_key.is_null());
  EXPECT_EQ(sal::crypto::key_algorithm::rsa, private_key.algorithm());
  EXPECT_EQ(256U, private_key.block_size());
}

//}}}1

TYPED_TEST(crypto_key_with_digest, sign_and_verify_signature) //{{{1
{
  std::error_code error;
  uint8_t signature_buf[1024];
  auto signature_size = this->private_key.sign(TypeParam(),
    this->case_name.cbegin(), this->case_name.cend(),
    std::begin(signature_buf), std::end(signature_buf),
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_EQ(this->private_key.block_size(), signature_size);

  auto is_valid = this->public_key.verify_signature(TypeParam(),
    this->case_name.cbegin(), this->case_name.cend(),
    signature_buf, signature_buf + signature_size,
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_TRUE(is_valid);

  EXPECT_NO_THROW(
    signature_size = this->private_key.sign(TypeParam(),
      this->case_name.cbegin(), this->case_name.cend(),
      std::begin(signature_buf), std::end(signature_buf)
    )
  );
  EXPECT_EQ(this->private_key.block_size(), signature_size);

  EXPECT_NO_THROW(
    is_valid = this->public_key.verify_signature(TypeParam(),
      this->case_name.cbegin(), this->case_name.cend(),
      signature_buf, signature_buf + signature_size
    )
  );
  EXPECT_TRUE(is_valid);
}


TYPED_TEST(crypto_key_with_digest, sign_and_verify_signature_vector) //{{{1
{
  auto signature = this->private_key.sign(TypeParam(), this->case_name);
  EXPECT_EQ(this->private_key.block_size(), signature.size());

  auto is_valid = this->public_key.verify_signature(TypeParam(),
    this->case_name,
    signature
  );
  EXPECT_TRUE(is_valid);
}


TYPED_TEST(crypto_key_with_digest, sign_with_null_private_key) //{{{1
{
  private_key_t key;
  std::error_code error;
  uint8_t signature[1024];

  key.sign(TypeParam(),
    std::cbegin(this->case_name), std::cend(this->case_name),
    std::begin(signature), std::end(signature),
    error
  );
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    key.sign(TypeParam(),
      std::cbegin(this->case_name), std::cend(this->case_name),
      std::begin(signature), std::end(signature)
    ),
    std::system_error
  );
}


TEST_F(crypto_key, sign_with_invalid_digest_type) //{{{1
{
  std::error_code error;
  uint8_t signature[1024];

  private_key.sign(sal::crypto::md5(),
    std::cbegin(case_name), std::cend(case_name),
    std::begin(signature), std::end(signature),
    error
  );
  EXPECT_EQ(std::errc::invalid_argument, error);

  EXPECT_THROW(
    private_key.sign(sal::crypto::md5(),
      std::cbegin(case_name), std::cend(case_name),
      std::begin(signature), std::end(signature)
    ),
    std::system_error
  );
}


TYPED_TEST(crypto_key_with_digest, sign_with_too_small_signature_buffer) //{{{1
{
  std::error_code error;
  uint8_t signature[1];

  this->private_key.sign(TypeParam(),
    std::cbegin(this->case_name), std::cend(this->case_name),
    std::begin(signature), std::end(signature),
    error
  );
  EXPECT_EQ(std::errc::result_out_of_range, error);

  EXPECT_THROW(
    this->private_key.sign(TypeParam(),
      std::cbegin(this->case_name), std::cend(this->case_name),
      std::begin(signature), std::end(signature)
    ),
    std::system_error
  );
}


TYPED_TEST(crypto_key_with_digest, sign_empty_buffer) //{{{1
{
  std::string empty_data;
  std::error_code error;
  uint8_t signature_buf[1024];
  auto signature_size = this->private_key.sign(TypeParam(),
    empty_data.cbegin(), empty_data.cend(),
    std::begin(signature_buf), std::end(signature_buf),
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_EQ(this->private_key.block_size(), signature_size);

  auto is_valid = this->public_key.verify_signature(TypeParam(),
    empty_data.cbegin(), empty_data.cend(),
    signature_buf, signature_buf + signature_size,
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_TRUE(is_valid);

  EXPECT_NO_THROW(
    signature_size = this->private_key.sign(TypeParam(),
      empty_data.cbegin(), empty_data.cend(),
      std::begin(signature_buf), std::end(signature_buf)
    )
  );
  EXPECT_EQ(this->private_key.block_size(), signature_size);

  EXPECT_NO_THROW(
    is_valid = this->public_key.verify_signature(TypeParam(),
      empty_data.cbegin(), empty_data.cend(),
      signature_buf, signature_buf + signature_size
    )
  );
  EXPECT_TRUE(is_valid);
}


TYPED_TEST(crypto_key_with_digest, sign_and_verify_signature_with_null_public_key) //{{{1
{
  std::error_code error;
  uint8_t signature[1024];

  auto size = this->private_key.sign(TypeParam(),
    std::cbegin(this->case_name), std::cend(this->case_name),
    std::begin(signature), std::end(signature),
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_EQ(this->private_key.block_size(), size);

  public_key_t key;
  (void)key.verify_signature(TypeParam(),
    this->case_name.cbegin(), this->case_name.cend(),
    signature, signature + size,
    error
  );
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    key.verify_signature(TypeParam(),
      this->case_name.cbegin(), this->case_name.cend(),
      signature, signature + size
    ),
    std::system_error
  );
}


TYPED_TEST(crypto_key_with_digest, sign_and_verify_signature_invalid_signature) //{{{1
{
  std::error_code error;
  uint8_t signature[1024];

  auto size = this->private_key.sign(TypeParam(),
    std::cbegin(this->case_name), std::cend(this->case_name),
    std::begin(signature), std::end(signature),
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_EQ(this->private_key.block_size(), size);

  // invalidate
  signature[0] ^= 1;

  auto is_valid = this->public_key.verify_signature(TypeParam(),
    this->case_name.cbegin(), this->case_name.cend(),
    signature, signature + size,
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_FALSE(is_valid);

  EXPECT_NO_THROW(
    is_valid = this->public_key.verify_signature(TypeParam(),
      this->case_name.cbegin(), this->case_name.cend(),
      signature, signature + size
    )
  );
  EXPECT_FALSE(is_valid);
}


TEST_F(crypto_key, verify_signature_with_different_digest_type) //{{{1
{
  std::error_code error;
  uint8_t signature[1024];

  auto size = private_key.sign(sal::crypto::sha1(),
    std::cbegin(case_name), std::cend(case_name),
    std::begin(signature), std::end(signature),
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_EQ(private_key.block_size(), size);

  auto is_valid = public_key.verify_signature(sal::crypto::sha512(),
    case_name.cbegin(), case_name.cend(),
    signature, signature + size,
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_FALSE(is_valid);

  EXPECT_NO_THROW(
    is_valid = public_key.verify_signature(sal::crypto::sha512(),
      case_name.cbegin(), case_name.cend(),
      signature, signature + size
    )
  );
  EXPECT_FALSE(is_valid);
}


TYPED_TEST(crypto_key_with_digest, verify_signature_with_invalid_digest) //{{{1
{
  std::error_code error;
  uint8_t signature[1024];

  auto size = this->private_key.sign(TypeParam(),
    std::cbegin(this->case_name), std::cend(this->case_name),
    std::begin(signature), std::end(signature),
    error
  );
  ASSERT_TRUE(!error) << error.message();
  EXPECT_EQ(this->private_key.block_size(), size);

  (void)this->public_key.verify_signature(sal::crypto::md5(),
    this->case_name.begin(), this->case_name.cend(),
    signature, signature + size,
    error
  );
  EXPECT_EQ(std::errc::invalid_argument, error);

  EXPECT_THROW(
    this->public_key.verify_signature(sal::crypto::md5(),
      this->case_name.begin(), this->case_name.cend(),
      signature, signature + size
    ),
    std::system_error
  );
}


//}}}1


} // namespace
