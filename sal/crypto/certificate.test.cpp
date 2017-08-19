#include <sal/crypto/certificate.hpp>
#include <sal/buf_ptr.hpp>
#include <sal/encode.hpp>
#include <sal/common.test.hpp>
#include <map>


namespace {


using namespace std::chrono_literals;

namespace oid = sal::crypto::oid;
using cert_t = sal::crypto::certificate_t;
using private_key_t = sal::crypto::private_key_t;


struct crypto_certificate
  : public sal_test::fixture
{
  static const std::string
    // generated chain
    root_cert,
    intermediate_cert,
    leaf_cert,

    // above as base64 encoded PKCS12 bytes
    chain_as_base64_pkcs12,
    chain_as_base64_pkcs12_no_passphrase,

    // specific testcase certificates
    cert_without_key_id,
    cert_with_generalized_time;


  static std::vector<uint8_t> to_der (const std::string &base64)
  {
    return sal::decode<sal::base64>(base64);
  }


  static std::string to_pem (const std::string &base64)
  {
    std::string result = "-----BEGIN CERTIFICATE-----\n";
    auto current_line_length = 0U;
    for (auto ch: base64)
    {
      result += ch;
      if (++current_line_length == 64)
      {
        result += '\n';
        current_line_length = 0;
      }
    }
    result += "\n-----END CERTIFICATE-----\n";
    return result;
  }
};


TEST_F(crypto_certificate, ctor) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());
}


TEST_F(crypto_certificate, ctor_copy) //{{{1
{
  auto a = cert_t::from_pem(to_pem(root_cert));
  EXPECT_FALSE(a.is_null());

  auto b = a;
  ASSERT_FALSE(b.is_null());
  ASSERT_FALSE(a.is_null());

  EXPECT_FALSE(a.serial_number().empty());
  EXPECT_FALSE(b.serial_number().empty());
  EXPECT_EQ(a.serial_number(), b.serial_number());
}


TEST_F(crypto_certificate, ctor_move) //{{{1
{
  auto a = cert_t::from_pem(to_pem(root_cert));
  EXPECT_FALSE(a.is_null());

  auto b = std::move(a);
  EXPECT_FALSE(b.is_null());
  EXPECT_FALSE(b.serial_number().empty());

  EXPECT_TRUE(a.is_null());
}


TEST_F(crypto_certificate, assign_copy) //{{{1
{
  auto a = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(a.is_null());

  auto b = cert_t::from_pem(to_pem(intermediate_cert));
  ASSERT_FALSE(b.is_null());

  EXPECT_FALSE(a.serial_number().empty());
  EXPECT_FALSE(b.serial_number().empty());
  EXPECT_NE(a.serial_number(), b.serial_number());

  b = a;
  ASSERT_FALSE(a.is_null());
  ASSERT_FALSE(b.is_null());

  EXPECT_FALSE(a.serial_number().empty());
  EXPECT_FALSE(b.serial_number().empty());
  EXPECT_EQ(a.serial_number(), b.serial_number());
}


TEST_F(crypto_certificate, assign_move) //{{{1
{
  auto a = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(a.is_null());

  auto b = cert_t::from_pem(to_pem(intermediate_cert));
  ASSERT_FALSE(b.is_null());

  EXPECT_FALSE(a.serial_number().empty());
  EXPECT_FALSE(b.serial_number().empty());
  EXPECT_NE(a.serial_number(), b.serial_number());

  auto b_serial_number_before_move = b.serial_number();
  b = std::move(a);
  ASSERT_FALSE(b.is_null());
  EXPECT_TRUE(a.is_null());

  EXPECT_FALSE(b.serial_number().empty());
  EXPECT_NE(b_serial_number_before_move, b.serial_number());
}


TEST_F(crypto_certificate, swap) //{{{1
{
  cert_t a, b = cert_t::from_pem(to_pem(root_cert));
  EXPECT_TRUE(a.is_null());
  EXPECT_FALSE(b.is_null());

  a.swap(b);
  EXPECT_FALSE(a.is_null());
  EXPECT_TRUE(b.is_null());
}


TEST_F(crypto_certificate, equals_true) //{{{1
{
  auto a = cert_t::from_pem(to_pem(root_cert)),
       b = cert_t::from_pem(to_pem(root_cert));
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}


TEST_F(crypto_certificate, equals_false) //{{{1
{
  auto a = cert_t::from_pem(to_pem(root_cert)),
       b = cert_t::from_pem(to_pem(leaf_cert));
  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a == b);
}


TEST_F(crypto_certificate, equals_one_null) //{{{1
{
  cert_t a = cert_t::from_pem(to_pem(root_cert)), b;

  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a == b);

  EXPECT_TRUE(b != a);
  EXPECT_FALSE(b == a);
}


TEST_F(crypto_certificate, equals_both_null) //{{{1
{
  cert_t a, b;
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}


TEST_F(crypto_certificate, version) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(!cert);
  EXPECT_EQ(3, cert.version());
}


TEST_F(crypto_certificate, version_from_null) //{{{1
{
  cert_t cert;
  ASSERT_TRUE(!cert);
  EXPECT_EQ(0, cert.version());
}


TEST_F(crypto_certificate, serial_number) //{{{1
{
  const std::pair<std::string, std::vector<uint8_t>> certs[] =
  {
    { root_cert,
      { 0x91, 0x02, 0xce, 0x0e, 0xc1, 0x7d, 0x4d, 0xce }

    },
    { intermediate_cert,
      { 0x10, 0x00 }
    },
    { leaf_cert,
      { 0x10, 0x01 }
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    auto serial_number = cert.serial_number(error);
    EXPECT_FALSE(serial_number.empty());
    EXPECT_TRUE(!error);
    EXPECT_EQ(cert_pem.second, serial_number);

    EXPECT_NO_THROW((void)cert.serial_number());
  }
}


TEST_F(crypto_certificate, serial_number_from_null) //{{{1
{
  cert_t cert;
  ASSERT_TRUE(!cert);

  std::error_code error;
  EXPECT_TRUE(cert.serial_number(error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.serial_number(),
    std::system_error
  );
}


TEST_F(crypto_certificate, digest) //{{{1
{
  const std::pair<std::string,  // pem
        std::pair<
          std::vector<uint8_t>, // sha1
          std::vector<uint8_t>  // sha256
        >> certs[] =
  {
    { root_cert,
      {
        {
          0x27, 0xb0, 0x6d, 0x90, 0xc1, 0xca, 0x29, 0xa7, 0xe2, 0x9b,
          0xc3, 0x53, 0x20, 0x9a, 0xa3, 0x4d, 0xac, 0x25, 0x88, 0x7d,
        },
        {
          0xec, 0x0e, 0x67, 0xba, 0x60, 0xed, 0x9d, 0x74,
          0x69, 0x38, 0xc6, 0x43, 0x38, 0x5e, 0x57, 0x02,
          0x62, 0xfb, 0x9f, 0x06, 0xff, 0x51, 0x55, 0xa9,
          0x64, 0x26, 0xd3, 0x6c, 0x2b, 0xd8, 0x7d, 0x69,
        }
      }
    },
    { intermediate_cert,
      {
        {
          0xdc, 0x59, 0x16, 0x1c, 0xa3, 0x91, 0x78, 0x4d, 0xaa, 0xde,
          0xb3, 0x27, 0xfb, 0x45, 0xcf, 0xc8, 0x08, 0x4f, 0xf7, 0x8d,
        },
        {
          0xa9, 0x30, 0x05, 0xa9, 0x01, 0x9e, 0x2c, 0xb3,
          0x5b, 0x69, 0x97, 0x77, 0x93, 0x21, 0x98, 0xd9,
          0xb7, 0xe1, 0x47, 0x25, 0x8f, 0x49, 0x5a, 0x21,
          0xd0, 0x24, 0xf3, 0xd8, 0x48, 0x45, 0x0c, 0x73,
        }
      }
    },
    { leaf_cert,
      {
        {
          0xef, 0xbe, 0x01, 0xb6, 0x43, 0x34, 0x57, 0xae, 0xf9, 0xfc,
          0x66, 0x06, 0x4d, 0xe2, 0x09, 0x50, 0xee, 0xb4, 0x10, 0x40,
        },
        {
          0x59, 0xdd, 0xc5, 0x22, 0xd6, 0x86, 0x78, 0x72,
          0xf3, 0x19, 0x11, 0xae, 0x5a, 0x4d, 0xb6, 0xbb,
          0xb7, 0xf2, 0xed, 0xe3, 0x5c, 0x99, 0x91, 0x79,
          0xec, 0x2a, 0x11, 0x37, 0xcc, 0x5a, 0xfd, 0x58,
        }
      }
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    auto digest = cert.digest<sal::crypto::sha1>(error);
    EXPECT_TRUE(!error);
    EXPECT_EQ(cert_pem.second.first, digest);

    digest = cert.digest<sal::crypto::sha256>(error);
    EXPECT_TRUE(!error);
    EXPECT_EQ(cert_pem.second.second, digest);

    EXPECT_NO_THROW((void)cert.digest<sal::crypto::sha1>());
  }
}


TEST_F(crypto_certificate, digest_from_null) //{{{1
{
  cert_t cert;
  ASSERT_TRUE(!cert);

  using digest_alg = sal::crypto::sha1;

  std::error_code error;
  EXPECT_TRUE(cert.digest<digest_alg>(error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.digest<digest_alg>(),
    std::system_error
  );
}


TEST_F(crypto_certificate, authority_key_identifier) //{{{1
{
  const std::pair<std::string, std::vector<uint8_t>> certs[] =
  {
    { root_cert,
      {
        0xcd, 0x81, 0x71, 0xa1, 0xf8, 0x82, 0xf6, 0x04, 0x95, 0x25,
        0x68, 0x81, 0x34, 0x77, 0x2d, 0xa9, 0x5a, 0x1f, 0xc3, 0x9c,
      }
    },
    { intermediate_cert,
      {
        0xcd, 0x81, 0x71, 0xa1, 0xf8, 0x82, 0xf6, 0x04, 0x95, 0x25,
        0x68, 0x81, 0x34, 0x77, 0x2d, 0xa9, 0x5a, 0x1f, 0xc3, 0x9c,
      }
    },
    { leaf_cert,
      {
        0x46, 0x43, 0xee, 0x6f, 0xbe, 0xed, 0x47, 0x01, 0x7d, 0x68,
        0x0c, 0x75, 0x3d, 0xe5, 0x47, 0x7e, 0x82, 0x24, 0xde, 0xb2,
      }
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(!cert);

    std::error_code error;
    auto id = cert.authority_key_identifier(error);
    ASSERT_TRUE(!error);
    EXPECT_EQ(cert_pem.second, id);

    EXPECT_NO_THROW((void)cert.authority_key_identifier());
  }
}


TEST_F(crypto_certificate, authority_key_identifier_from_null) //{{{1
{
  cert_t cert;
  ASSERT_TRUE(!cert);

  std::error_code error;
  EXPECT_TRUE(cert.authority_key_identifier(error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.authority_key_identifier(),
    std::system_error
  );
}


TEST_F(crypto_certificate, authority_key_identifier_none) //{{{1
{
  cert_t cert = cert_t::from_pem(to_pem(cert_without_key_id));
  ASSERT_FALSE(!cert);

  std::error_code error;
  EXPECT_TRUE(cert.authority_key_identifier(error).empty());
  EXPECT_TRUE(!error);

  EXPECT_NO_THROW(
    (void)cert.authority_key_identifier()
  );
}


TEST_F(crypto_certificate, subject_key_identifier) //{{{1
{
  const std::pair<std::string, std::vector<uint8_t>> certs[] =
  {
    { root_cert,
      {
        0xcd, 0x81, 0x71, 0xa1, 0xf8, 0x82, 0xf6, 0x04, 0x95, 0x25,
        0x68, 0x81, 0x34, 0x77, 0x2d, 0xa9, 0x5a, 0x1f, 0xc3, 0x9c,
      }
    },
    { intermediate_cert,
      {
        0x46, 0x43, 0xee, 0x6f, 0xbe, 0xed, 0x47, 0x01, 0x7d, 0x68,
        0x0c, 0x75, 0x3d, 0xe5, 0x47, 0x7e, 0x82, 0x24, 0xde, 0xb2,
      }
    },
    { leaf_cert,
      {
        0xd8, 0x45, 0x6f, 0xd8, 0x5b, 0x0b, 0x1e, 0x7a, 0x26, 0x11,
        0xb8, 0x1c, 0xda, 0xdf, 0xfc, 0x7b, 0xfc, 0xad, 0x31, 0x85,
      }
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(!cert);

    std::error_code error;
    auto id = cert.subject_key_identifier(error);
    ASSERT_TRUE(!error);
    EXPECT_EQ(cert_pem.second, id);

    EXPECT_NO_THROW((void)cert.subject_key_identifier());
  }
}


TEST_F(crypto_certificate, subject_key_identifier_from_null) //{{{1
{
  cert_t cert;
  ASSERT_TRUE(!cert);

  std::error_code error;
  EXPECT_TRUE(cert.subject_key_identifier(error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.subject_key_identifier(),
    std::system_error
  );
}


TEST_F(crypto_certificate, subject_key_identifier_none) //{{{1
{
  cert_t cert = cert_t::from_pem(to_pem(cert_without_key_id));
  ASSERT_FALSE(!cert);

  std::error_code error;
  EXPECT_TRUE(cert.subject_key_identifier(error).empty());
  EXPECT_TRUE(!error);

  EXPECT_NO_THROW(
    (void)cert.subject_key_identifier()
  );
}


TEST_F(crypto_certificate, not_before) //{{{1
{
  const std::string certs[] =
  {
    root_cert,
    intermediate_cert,
    leaf_cert,
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem));
    ASSERT_FALSE(!cert);

    std::error_code error;
    auto not_before = cert.not_before(error);
    EXPECT_TRUE(!error);

    auto at = sal::now();
    EXPECT_GT(at, not_before);

    at -= 30 * 365 * 24h;
    EXPECT_LT(at, not_before);

    EXPECT_NO_THROW((void)cert.not_before());
  }
}


TEST_F(crypto_certificate, not_before_null) //{{{1
{
  cert_t cert;
  ASSERT_TRUE(!cert);

  std::error_code error;
  (void)cert.not_before(error);
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.not_before(),
    std::system_error
  );
}


TEST_F(crypto_certificate, not_after) //{{{1
{
  const std::string certs[] =
  {
    root_cert,
    intermediate_cert,
    leaf_cert,
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem));
    ASSERT_FALSE(!cert);

    std::error_code error;
    auto not_after = cert.not_after(error);
    EXPECT_TRUE(!error);

    auto at = sal::now();
    EXPECT_LT(at, not_after);

    at += 30 * 365 * 24h;
    EXPECT_GT(at, not_after);

    EXPECT_NO_THROW((void)cert.not_after());
  }
}


TEST_F(crypto_certificate, not_after_null) //{{{1
{
  cert_t cert;
  ASSERT_TRUE(!cert);

  std::error_code error;
  (void)cert.not_after(error);
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.not_after(),
    std::system_error
  );
}


TEST_F(crypto_certificate, not_after_with_generalized_time)
{
  auto cert = cert_t::from_pem(to_pem(cert_with_generalized_time));
  ASSERT_FALSE(!cert);

  std::error_code error;
  (void)cert.not_after(error);
  EXPECT_TRUE(!error);

  EXPECT_NO_THROW(cert.not_after());
}


TEST_F(crypto_certificate, not_expired) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(!cert);

  // now
  EXPECT_TRUE(cert.not_expired());

  // from now within 1 year
  EXPECT_TRUE(cert.not_expired(365 * 24h));

  // from tomorrow within 1 year
  EXPECT_TRUE(cert.not_expired(365 * 24h, sal::now() + 24h));
}


TEST_F(crypto_certificate, not_expired_null) //{{{1
{
  cert_t cert;
  ASSERT_TRUE(!cert);

  EXPECT_THROW(
    cert.not_expired(),
    std::system_error
  );

  EXPECT_THROW(
    cert.not_expired(1h),
    std::system_error
  );
}


TEST_F(crypto_certificate, not_expired_past) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(!cert);

  // 30 years in past
  auto past = sal::now() - 30 * 365 * 24h;
  EXPECT_FALSE(cert.not_expired(past));
  EXPECT_FALSE(cert.not_expired(365 * 24h, past));
}


TEST_F(crypto_certificate, not_expired_future) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(!cert);

  // 30 years in future
  auto future = sal::now() + 30 * 365 * 24h;
  EXPECT_FALSE(cert.not_expired(future));
  EXPECT_FALSE(cert.not_expired(365 * 24h, future));
}


TEST_F(crypto_certificate, issued_by) //{{{1
{
  const std::pair<std::string, std::string> certs[] =
  {
    { root_cert, root_cert },
    { intermediate_cert, root_cert },
    { leaf_cert, intermediate_cert },
  };

  for (const auto &pair: certs)
  {
    auto this_cert = cert_t::from_pem(to_pem(pair.first));
    ASSERT_FALSE(this_cert.is_null());

    auto issuer_cert = cert_t::from_pem(to_pem(pair.second));
    ASSERT_FALSE(issuer_cert.is_null());

    std::error_code error;
    EXPECT_TRUE(this_cert.issued_by(issuer_cert, error));
    EXPECT_TRUE(!error);

    EXPECT_NO_THROW((void)this_cert.issued_by(issuer_cert));
  }
}


TEST_F(crypto_certificate, issued_by_leaf) //{{{1
{
  const std::string certs[] =
  {
    root_cert,
    intermediate_cert,
    leaf_cert,
  };

  auto issuer_cert = cert_t::from_pem(to_pem(leaf_cert));
  ASSERT_FALSE(issuer_cert.is_null());

  for (const auto &cert_pem: certs)
  {
    auto this_cert = cert_t::from_pem(to_pem(cert_pem));
    ASSERT_FALSE(this_cert.is_null());

    std::error_code error;
    EXPECT_FALSE(this_cert.issued_by(issuer_cert, error));
    EXPECT_TRUE(!error);

    EXPECT_NO_THROW((void)this_cert.issued_by(issuer_cert));
  }
}


TEST_F(crypto_certificate, issued_by_null_cert) //{{{1
{
  cert_t this_cert, issuer_cert = cert_t::from_pem(to_pem(root_cert));
  ASSERT_TRUE(this_cert.is_null());
  ASSERT_FALSE(issuer_cert.is_null());

  std::error_code error;
  (void)this_cert.issued_by(issuer_cert, error);
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)this_cert.issued_by(issuer_cert),
    std::system_error
  );
}


TEST_F(crypto_certificate, issued_by_null_issuer) //{{{1
{
  cert_t this_cert = cert_t::from_pem(to_pem(root_cert)), issuer_cert;
  ASSERT_FALSE(this_cert.is_null());
  ASSERT_TRUE(issuer_cert.is_null());

  std::error_code error;
  (void)this_cert.issued_by(issuer_cert, error);
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)this_cert.issued_by(issuer_cert),
    std::system_error
  );
}


TEST_F(crypto_certificate, is_self_signed) //{{{1
{
  const std::pair<std::string, bool> certs[] =
  {
    { root_cert, true },
    { intermediate_cert, false },
    { leaf_cert, false },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    EXPECT_EQ(cert_pem.second, cert.is_self_signed(error));
    EXPECT_TRUE(!error);

    EXPECT_NO_THROW((void)cert.is_self_signed());
  }
}


TEST_F(crypto_certificate, is_self_signed_from_null) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());

  std::error_code error;
  (void)cert.is_self_signed(error);
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.is_self_signed(),
    std::system_error
  );
}


TEST_F(crypto_certificate, issuer) //{{{1
{
  const std::pair<std::string, cert_t::distinguished_name_t> certs[] =
  {
    { root_cert,
      {
        { oid::country_name, "EE" },
        { oid::state_or_province_name, "Estonia" },
        { oid::organization_name, "SAL" },
        { oid::organizational_unit_name, "SAL CA" },
        { oid::common_name, "SAL Root CA" },
      }
    },
    { intermediate_cert,
      {
        { oid::country_name, "EE" },
        { oid::state_or_province_name, "Estonia" },
        { oid::organization_name, "SAL" },
        { oid::organizational_unit_name, "SAL CA" },
        { oid::common_name, "SAL Root CA" },
      }
    },
    { leaf_cert,
      {
        { oid::country_name, "EE" },
        { oid::state_or_province_name, "Estonia" },
        { oid::organization_name, "SAL" },
        { oid::organizational_unit_name, "SAL CA" },
        { oid::common_name, "SAL Intermediate CA" },
      }
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    auto issuer = cert.issuer(error);
    ASSERT_TRUE(!error);
    EXPECT_EQ(cert_pem.second, issuer);

    EXPECT_NO_THROW((void)cert.issuer());
  }
}


TEST_F(crypto_certificate, issuer_from_null) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());

  std::error_code error;
  EXPECT_TRUE(cert.issuer(error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.issuer(),
    std::system_error
  );
}


TEST_F(crypto_certificate, issuer_with_oid) //{{{1
{
  const std::pair<std::string, cert_t::distinguished_name_t> certs[] =
  {
    { root_cert, { { oid::common_name, "SAL Root CA" }, } },
    { intermediate_cert, { { oid::common_name, "SAL Root CA" }, } },
    { leaf_cert, { { oid::common_name, "SAL Intermediate CA" }, } },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    auto issuer = cert.issuer(oid::common_name, error);
    ASSERT_TRUE(!error);
    EXPECT_EQ(cert_pem.second, issuer);

    EXPECT_NO_THROW((void)cert.issuer(oid::common_name));
  }
}


TEST_F(crypto_certificate, issuer_with_oid_from_null) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());

  std::error_code error;
  EXPECT_TRUE(cert.issuer(oid::common_name, error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.issuer(oid::common_name),
    std::system_error
  );
}


TEST_F(crypto_certificate, issuer_with_oid_missing) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(cert.is_null());

  std::error_code error;
  auto issuer = cert.issuer(oid::given_name, error);
  ASSERT_TRUE(!error);
  EXPECT_TRUE(issuer.empty());

  EXPECT_NO_THROW(
    EXPECT_TRUE(cert.issuer(oid::given_name).empty())
  );
}


TEST_F(crypto_certificate, issuer_with_oid_invalid) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(cert.is_null());

  std::error_code error;
  auto issuer = cert.issuer(case_name, error);
  ASSERT_TRUE(!error);
  EXPECT_TRUE(issuer.empty());

  EXPECT_NO_THROW(
    EXPECT_TRUE(cert.issuer(case_name).empty())
  );
}


TEST_F(crypto_certificate, subject) //{{{1
{
  const std::pair<std::string, cert_t::distinguished_name_t> certs[] =
  {
    { root_cert,
      {
        { oid::country_name, "EE" },
        { oid::state_or_province_name, "Estonia" },
        { oid::organization_name, "SAL" },
        { oid::organizational_unit_name, "SAL CA" },
        { oid::common_name, "SAL Root CA" },
      }
    },
    { intermediate_cert,
      {
        { oid::country_name, "EE" },
        { oid::state_or_province_name, "Estonia" },
        { oid::organization_name, "SAL" },
        { oid::organizational_unit_name, "SAL CA" },
        { oid::common_name, "SAL Intermediate CA" },
      }
    },
    { leaf_cert,
      {
        { oid::country_name, "EE" },
        { oid::state_or_province_name, "Estonia" },
        { oid::organization_name, "SAL" },
        { oid::organizational_unit_name, "SAL Test" },
        { oid::common_name, "test.sal.ee" },
      }
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    auto subject = cert.subject(error);
    ASSERT_TRUE(!error);
    EXPECT_EQ(cert_pem.second, subject);

    EXPECT_NO_THROW((void)cert.subject());
  }
}


TEST_F(crypto_certificate, subject_from_null) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());

  std::error_code error;
  EXPECT_TRUE(cert.subject(error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.subject(),
    std::system_error
  );
}


TEST_F(crypto_certificate, subject_with_oid) //{{{1
{
  const std::pair<std::string, cert_t::distinguished_name_t> certs[] =
  {
    { root_cert, { { oid::common_name, "SAL Root CA" }, } },
    { intermediate_cert, { { oid::common_name, "SAL Intermediate CA" }, } },
    { leaf_cert, { { oid::common_name, "test.sal.ee" }, } },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    auto subject = cert.subject(oid::common_name, error);
    ASSERT_TRUE(!error);
    EXPECT_EQ(cert_pem.second, subject);

    EXPECT_NO_THROW((void)cert.subject(oid::common_name));
  }
}


TEST_F(crypto_certificate, subject_with_oid_from_null) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());

  std::error_code error;
  EXPECT_TRUE(cert.subject(oid::common_name, error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.subject(oid::common_name),
    std::system_error
  );
}


TEST_F(crypto_certificate, subject_with_oid_missing) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(cert.is_null());

  std::error_code error;
  auto subject = cert.subject(oid::given_name, error);
  ASSERT_TRUE(!error);
  EXPECT_TRUE(subject.empty());

  EXPECT_NO_THROW(
    EXPECT_TRUE(cert.subject(oid::given_name).empty())
  );
}


TEST_F(crypto_certificate, subject_with_oid_invalid) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(cert.is_null());

  std::error_code error;
  auto subject = cert.subject(case_name, error);
  ASSERT_TRUE(!error);
  EXPECT_TRUE(subject.empty());

  EXPECT_NO_THROW(
    EXPECT_TRUE(cert.subject(case_name).empty())
  );
}


TEST_F(crypto_certificate, issuer_alt_names) //{{{1
{
  const std::pair<std::string, std::vector<std::pair<cert_t::alt_name, std::string>>> certs[] =
  {
    { root_cert,
      { }
    },
    { intermediate_cert,
      { }
    },
    { leaf_cert,
      {
        { cert_t::alt_name::dns,   "ca.sal.alt.ee" },
        { cert_t::alt_name::uri,   "https://ca.sal.alt.ee/path" },
      }
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    auto alt_name = cert.issuer_alt_names(error);
    ASSERT_TRUE(!error);
    EXPECT_EQ(cert_pem.second, alt_name);

    EXPECT_NO_THROW((void)cert.issuer_alt_names());
  }
}


TEST_F(crypto_certificate, issuer_alt_name_from_null) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());

  std::error_code error;
  EXPECT_TRUE(cert.issuer_alt_names(error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.issuer_alt_names(),
    std::system_error
  );
}


TEST_F(crypto_certificate, subject_alt_names) //{{{1
{
  const std::pair<std::string, std::vector<std::pair<cert_t::alt_name, std::string>>> certs[] =
  {
    { root_cert,
      { }
    },
    { intermediate_cert,
      { }
    },
    { leaf_cert,
      {
        { cert_t::alt_name::ip,    "1.2.3.4" },
        { cert_t::alt_name::ip,    "2001:db8:85a3::8a2e:370:7334" },
        { cert_t::alt_name::dns,   "*.sal.alt.ee" },
        { cert_t::alt_name::dns,   "sal.alt.ee" },
        { cert_t::alt_name::email, "sal@alt.ee" },
        { cert_t::alt_name::uri,   "https://sal.alt.ee/path" },
      }
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    auto alt_name = cert.subject_alt_names(error);
    ASSERT_TRUE(!error);
    EXPECT_EQ(cert_pem.second, alt_name);

    EXPECT_NO_THROW((void)cert.subject_alt_names());
  }
}


TEST_F(crypto_certificate, subject_alt_name_from_null) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());

  std::error_code error;
  EXPECT_TRUE(cert.subject_alt_names(error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.subject_alt_names(),
    std::system_error
  );
}


TEST_F(crypto_certificate, public_key) //{{{1
{
  const std::pair<std::string, std::vector<uint8_t>> certs[] =
  {
    { root_cert,
      { }
    },
    { intermediate_cert,
      { }
    },
    { leaf_cert,
      { }
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    auto key = cert.public_key(error);
    ASSERT_TRUE(!error);
    ASSERT_FALSE(!key);

    EXPECT_NO_THROW((void)cert.public_key());
  }
}


TEST_F(crypto_certificate, public_key_from_null) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());

  std::error_code error;
  auto key = cert.public_key(error);
  ASSERT_EQ(std::errc::bad_address, error);
  ASSERT_TRUE(key.is_null());

  EXPECT_THROW(
    (void)cert.public_key(),
    std::system_error
  );
}


TEST_F(crypto_certificate, to_der) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));

  uint8_t data[8192];
  std::error_code error;
  auto data_end = cert.to_der(sal::make_buf(data), error);
  ASSERT_TRUE(!error);
  ASSERT_NE(nullptr, data_end);
  std::vector<uint8_t> der(data, data_end);

  auto expected_der = to_der(root_cert);
  EXPECT_EQ(expected_der, der);

  EXPECT_NO_THROW(
    (void)cert.to_der(sal::make_buf(data))
  );
}


TEST_F(crypto_certificate, to_der_from_null) //{{{1
{
  cert_t cert;
  ASSERT_TRUE(cert.is_null());

  uint8_t data[8192];
  std::error_code error;
  EXPECT_EQ(nullptr, cert.to_der(sal::make_buf(data), error));
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.to_der(sal::make_buf(data)),
    std::system_error
  );
}


TEST_F(crypto_certificate, to_der_result_exact_range) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(cert.is_null());

  uint8_t data[8192];
  std::error_code error;
  auto data_end = cert.to_der(sal::make_buf(data), error);
  ASSERT_TRUE(!error);
  ASSERT_NE(nullptr, data_end);

  auto end = cert.to_der(sal::make_buf(data, data_end - data), error);
  EXPECT_TRUE(!error);
  EXPECT_EQ(data_end, end);
}


TEST_F(crypto_certificate, to_der_result_out_of_range) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));
  ASSERT_FALSE(cert.is_null());

  uint8_t data[1];
  std::error_code error;
  EXPECT_EQ(nullptr, cert.to_der(sal::make_buf(data), error));
  EXPECT_EQ(std::errc::result_out_of_range, error);

  EXPECT_THROW(
    (void)cert.to_der(sal::make_buf(data)),
    std::system_error
  );
}


TEST_F(crypto_certificate, to_der_vector) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(root_cert));

  std::error_code error;
  auto der = cert.to_der(error);
  ASSERT_TRUE(!error);

  auto expected_der = to_der(root_cert);
  EXPECT_EQ(expected_der, der);

  EXPECT_NO_THROW((void)cert.to_der());
}


TEST_F(crypto_certificate, to_der_vector_from_null) //{{{1
{
  cert_t cert;
  ASSERT_TRUE(cert.is_null());

  std::error_code error;
  EXPECT_TRUE(cert.to_der(error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.to_der(),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_der) //{{{1
{
  auto data = to_der(root_cert);

  std::error_code error;
  auto cert = cert_t::from_der(data, error);
  ASSERT_TRUE(!error) << error.message() << " (" << error.value() << ')';
  EXPECT_FALSE(cert.is_null());

  EXPECT_NO_THROW(cert_t::from_der(data));
}


TEST_F(crypto_certificate, from_der_empty_data) //{{{1
{
  std::vector<uint8_t> data;

  std::error_code error;
  auto cert = cert_t::from_der(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_der(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_der_insufficient_data) //{{{1
{
  auto data = to_der(root_cert);
  data.resize(data.size() / 2);

  std::error_code error;
  auto cert = cert_t::from_der(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_der(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_der_invalid_data) //{{{1
{
  auto data = to_der(root_cert);
  data[0] = 'X';

  std::error_code error;
  auto cert = cert_t::from_der(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_der(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem) //{{{1
{
  auto data = root_cert;

  std::error_code error;
  auto cert = cert_t::from_pem(to_pem(data), error);
  ASSERT_TRUE(!error) << error.message() << " (" << error.value() << ')';
  EXPECT_FALSE(cert.is_null());

  EXPECT_NO_THROW(cert_t::from_pem(to_pem(data)));
}


TEST_F(crypto_certificate, from_pem_empty_data) //{{{1
{
  std::string data;

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error);
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_insufficient_data) //{{{1
{
  auto data = root_cert;
  data.resize(data.size() / 2);
  data = to_pem(data);

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_invalid_envelope) //{{{1
{
  auto data = to_pem(root_cert);
  data[0] = 'X';

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_only_header) //{{{1
{
  std::string data = "-----BEGIN CERTIFICATE-----\n";

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_without_footer) //{{{1
{
  auto data = to_pem(root_cert);
  auto footer = data.find("-----END");
  ASSERT_NE(data.npos, footer);
  data.erase(footer);

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_only_envelope) //{{{1
{
  std::string data =
    "-----BEGIN CERTIFICATE-----\n"
    "-----END CERTIFICATE-----";

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}

TEST_F(crypto_certificate, from_pem_partial_envelope_without_data) //{{{1
{
  std::string data =
    "-----BEGIN"
    "-----END";

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}



TEST_F(crypto_certificate, from_pem_invalid_data) //{{{1
{
  auto data = to_pem(root_cert + 'X');

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_too_much_data) //{{{1
{
  // using internal knowledge: max 16kB PEM is accepted
  auto raw_data = to_der(root_cert);
  while (raw_data.size() <= 16 * 1024)
  {
    raw_data.insert(raw_data.end(), raw_data.begin(), raw_data.end());
  }
  auto data = to_pem(sal::encode<sal::base64>(raw_data));

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, import_pkcs12) //{{{1
{
  auto pkcs12 = to_der(chain_as_base64_pkcs12);

  std::error_code error;
  std::vector<cert_t> chain;
  private_key_t private_key;
  auto cert = sal::crypto::import_pkcs12(pkcs12, "TestPassword",
    private_key, chain, error
  );
  ASSERT_TRUE(!error) << error.message();

  EXPECT_FALSE(!cert);
  EXPECT_FALSE(!private_key);

  ASSERT_EQ(2U, chain.size());
  EXPECT_EQ(
    cert_t::from_pem(to_pem(intermediate_cert)),
    chain[0]
  );
  EXPECT_EQ(
    cert_t::from_pem(to_pem(root_cert)),
    chain[1]
  );

  EXPECT_NO_THROW(
    sal::crypto::import_pkcs12(pkcs12, "TestPassword", private_key, chain)
  );
}


TEST_F(crypto_certificate, import_pkcs12_without_private_key) //{{{1
{
  auto pkcs12 = to_der(chain_as_base64_pkcs12);

  std::error_code error;
  std::vector<cert_t> chain;
  auto cert = sal::crypto::import_pkcs12(pkcs12, "TestPassword",
    chain, error
  );
  ASSERT_TRUE(!error) << error.message();

  EXPECT_FALSE(!cert);
  EXPECT_EQ(2U, chain.size());

  EXPECT_NO_THROW(
    sal::crypto::import_pkcs12(pkcs12, "TestPassword", chain)
  );
}


TEST_F(crypto_certificate, import_pkcs12_without_chain) //{{{1
{
  auto pkcs12 = to_der(chain_as_base64_pkcs12);

  std::error_code error;
  private_key_t private_key;
  auto cert = sal::crypto::import_pkcs12(pkcs12, "TestPassword",
    private_key, error
  );
  ASSERT_TRUE(!error) << error.message();

  EXPECT_FALSE(!cert);
  EXPECT_FALSE(!private_key);

  EXPECT_NO_THROW(
    sal::crypto::import_pkcs12(pkcs12, "TestPassword", private_key)
  );
}


TEST_F(crypto_certificate, import_pkcs12_no_data) //{{{1
{
  std::vector<uint8_t> pkcs12;

  std::error_code error;
  auto cert = sal::crypto::import_pkcs12(pkcs12, "TestPassword", error);
  EXPECT_FALSE(!error);
  EXPECT_TRUE(!cert);

  EXPECT_THROW(
    (void)sal::crypto::import_pkcs12(pkcs12, "TestPassword"),
    std::system_error
  );
}


TEST_F(crypto_certificate, import_pkcs12_partial_data) //{{{1
{
  auto pkcs12 = to_der(chain_as_base64_pkcs12);
  pkcs12.resize(pkcs12.size() / 2);

  std::error_code error;
  auto cert = sal::crypto::import_pkcs12(pkcs12, "TestPassword", error);
  EXPECT_FALSE(!error);
  EXPECT_TRUE(!cert);

  EXPECT_THROW(
    (void)sal::crypto::import_pkcs12(pkcs12, "TestPassword"),
    std::system_error
  );
}


TEST_F(crypto_certificate, import_pkcs12_invalid_data) //{{{1
{
  auto pkcs12 = to_der(chain_as_base64_pkcs12);
  for (auto &b: pkcs12)
  {
    b ^= 1;
  }

  std::error_code error;
  auto cert = sal::crypto::import_pkcs12(pkcs12, "TestPassword", error);
  EXPECT_FALSE(!error);
  EXPECT_TRUE(!cert);

  EXPECT_THROW(
    (void)sal::crypto::import_pkcs12(pkcs12, "TestPassword"),
    std::system_error
  );
}


TEST_F(crypto_certificate, import_pkcs12_no_passphrase) //{{{1
{
  auto pkcs12 = to_der(chain_as_base64_pkcs12);

  std::error_code error;
  auto cert = sal::crypto::import_pkcs12(pkcs12, "", error);
  EXPECT_FALSE(!error);
  EXPECT_TRUE(!cert);

  EXPECT_THROW(
    (void)sal::crypto::import_pkcs12(pkcs12, ""),
    std::system_error
  );
}


#if __sal_os_darwin
TEST_F(crypto_certificate, DISABLED_import_pkcs12_valid_no_passphrase) //{{{1
#else
TEST_F(crypto_certificate, import_pkcs12_valid_no_passphrase) //{{{1
#endif
{
  auto pkcs12 = to_der(chain_as_base64_pkcs12_no_passphrase);

  std::error_code error;
  std::vector<cert_t> chain;
  private_key_t private_key;
  auto cert = sal::crypto::import_pkcs12(pkcs12, "",
    private_key, chain, error
  );
  ASSERT_TRUE(!error) << error.message();

  EXPECT_FALSE(!cert);
  EXPECT_FALSE(!private_key);
  EXPECT_EQ(2U, chain.size());

  EXPECT_NO_THROW(
    sal::crypto::import_pkcs12(pkcs12, "", private_key, chain)
  );
}


TEST_F(crypto_certificate, import_pkcs12_invalid_passphrase) //{{{1
{
  auto pkcs12 = to_der(chain_as_base64_pkcs12);

  std::error_code error;
  auto cert = sal::crypto::import_pkcs12(pkcs12, case_name, error);
  EXPECT_FALSE(!error);
  EXPECT_TRUE(!cert);

  EXPECT_THROW(
    (void)sal::crypto::import_pkcs12(pkcs12, case_name),
    std::system_error
  );
}


//}}}1

/* see scripts/make_ca.sh
Not Before: Aug  7 17:26:xx 2017 GMT
Not After : Jul  3 17:26:xx 2037 GMT

pkcs12 bundle as base64 generated with:
cat server.key.pem server.pem intermediate.pem ca.pem \
  | openssl pkcs12 -export -passin pass:ServerPassword -passout pass:TestPassword \
  | openssl base64
*/

const std::string crypto_certificate::root_cert = // {{{1
  "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";

const std::string crypto_certificate::intermediate_cert = // {{{1
  "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";

const std::string crypto_certificate::leaf_cert = // {{{1
  "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"
  "QOXpu3kF+2btU7ro9vJPcWs=";

const std::string crypto_certificate::chain_as_base64_pkcs12 = // {{{1
  "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"
  "AgICCAA=";

const std::string crypto_certificate::chain_as_base64_pkcs12_no_passphrase = // {{{1
  "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"
  "hgICCAA=";


const std::string crypto_certificate::cert_without_key_id = // {{{1
  "MIIEczCCAlugAwIBAgICEAEwDQYJKoZIhvcNAQELBQAwXDELMAkGA1UEBhMCRUUx"
  "EDAOBgNVBAgMB0VzdG9uaWExDDAKBgNVBAoMA1NBTDEPMA0GA1UECwwGU0FMIENB"
  "MRwwGgYDVQQDDBNTQUwgSW50ZXJtZWRpYXRlIENBMB4XDTE3MDgwNTE3NDM0MFoX"
  "DTM3MDcwMTE3NDM0MFowVjELMAkGA1UEBhMCRUUxEDAOBgNVBAgMB0VzdG9uaWEx"
  "DDAKBgNVBAoMA1NBTDERMA8GA1UECwwIU0FMIFRlc3QxFDASBgNVBAMMC3Rlc3Qu"
  "c2FsLmVlMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxIo08Ex4zQEW"
  "PML0MailnwLzNCUF5CXjMvemrow+ecSSG9XgRHCTZcvVF1zBLMZdx7B6g69Qhkp4"
  "mCRLceXhYhCfByL/j6qYDMjhUOuFQ3snzQHaghtK+qj86QqnYx9JWsCnww4iLwJ5"
  "gG/wbo+53cB20EyE1Gb3DSdN3OuAUid+K4AldQZCLCheT3X4nHj1q8jouqcvNQzb"
  "+oQso486gmr7cjuPZjBeNUSnK8y37VV2MLSnbneo/yd/c9MGlrKr9saj+Kdlba/r"
  "7Z+QBDkGstahFIohmiZYADAm4xnOQbCuumstouvHxQKAWqzsBLcuKfwRvXwkOh8V"
  "edR9oV8LSQIDAQABo0UwQzAJBgNVHRMEAjAAMBEGCWCGSAGG+EIBAQQEAwIGQDAO"
  "BgNVHQ8BAf8EBAMCBaAwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDQYJKoZIhvcNAQEL"
  "BQADggIBACahCmH39B9x9EReBNZRhN64d7y9JnaDFSGGoDWl7dApXfPwLpJu55dd"
  "41N6u6UrOBsBYX5AOTacMzDsPRNn/qSc1crOcu+seyZatzsmIXhgK2JQnPgatjEI"
  "+HCntZ2TGaQnTsQc14BlWNN9PKtj5RhZKqA8DihPzIyIpCagoUnDMkTnMt6GuXdt"
  "9qknAyjR91NNkZnnap6AHZFGj/RYQmxzpzXRnAeZUsLjTJ0nDL+c9ooEXkfH+EEX"
  "2qK/j0asNsg/yaJ1DwBC6AWKI+HHLVObbKsa1IgfisCiq/qBA1nvmcYMSZIjF8WV"
  "SoOIxhJL3L7C4QbxccT3lcgCHGVFwxkVsSiJxyS+Javf9YBfBElpPmf90gxqMM9M"
  "wa0CVvFZCgIjHbNzANn/mkYsg8U+Sux39BbKEni58Ds+QUXZXFrhL8Jb/75bGv8S"
  "Wmt+SwKNg6dO10oxx/TdvwywcM/rYHE6EaGiyNUCB8IY3t/9DPFCj6S7hWPaCAn6"
  "sfg/V2SJ9f07HtNgcGKMkAOwFJUhMLASz46ns0Bk29ewOsmqVIAsug4r40biMG6x"
  "Di2iFMdIDSZlfaWfOyCEF5E2O3H5itDnmvDb+f/Z8gRVLWbN1XsYDULaacEmHzAE"
  "mJ9jLcEZFj1nEsX2o6hFPKUGIy01e6MlMtOnSxiiCq5LikfNvgmi";

const std::string crypto_certificate::cert_with_generalized_time = // {{{1
  "MIIC4DCCAkmgAwIBAgIJAJVaNiMqdm70MA0GCSqGSIb3DQEBBQUAMFQxCzAJBgNV"
  "BAYTAkVFMRAwDgYDVQQIEwdFc3RvbmlhMQwwCgYDVQQKEwNTQUwxDzANBgNVBAsT"
  "BlNBTCBDQTEUMBIGA1UEAxMLU0FMIFJvb3QgQ0EwIBcNMTcwODE2MTQ0NDM3WhgP"
  "MjExNzA3MjMxNDQ0MzdaMFQxCzAJBgNVBAYTAkVFMRAwDgYDVQQIEwdFc3Rvbmlh"
  "MQwwCgYDVQQKEwNTQUwxDzANBgNVBAsTBlNBTCBDQTEUMBIGA1UEAxMLU0FMIFJv"
  "b3QgQ0EwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAOR1MTVq798PWzMZ82EX"
  "pQPUMZpJsUC1CpgvHfMfroExkiigOnyI1ZUJCdf3MIUP/KDS7Cn+ITTh9Cm7za+J"
  "YruFKYw12XjJFalD8bzLuBT5UJ45CRhCZZS+YFVllSpTp4qG4kJgIwna5oUd+pr1"
  "+RQ/UzfQsV6bkSvTLf2Tr6fbAgMBAAGjgbcwgbQwHQYDVR0OBBYEFBYUixO2Db6a"
  "w8Ykp634qEMSQSNnMIGEBgNVHSMEfTB7gBQWFIsTtg2+msPGJKet+KhDEkEjZ6FY"
  "pFYwVDELMAkGA1UEBhMCRUUxEDAOBgNVBAgTB0VzdG9uaWExDDAKBgNVBAoTA1NB"
  "TDEPMA0GA1UECxMGU0FMIENBMRQwEgYDVQQDEwtTQUwgUm9vdCBDQYIJAJVaNiMq"
  "dm70MAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEFBQADgYEAti5U6NnqaFrfpYH2"
  "IPw6dmZkSPETfTB3G4xNFeS2+xj02V+TOtTiF2k2nQy/OGP3nX7dTDjPogvV54ZK"
  "vsZdyWtugSlBmzc0+40GJ5l8c4aiwqdjz5Xc7l9Zd5TI8J5+gM1vf5L0apFn0tu/"
  "0ZMVcJiK7QldCk/RsD3FL8H5nEs=";

// }}}

} // namespace
