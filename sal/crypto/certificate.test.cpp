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

  ASSERT_FALSE(!cert);
  EXPECT_EQ(cert_t::from_pem(to_pem(leaf_cert)), cert);

  EXPECT_FALSE(!private_key);

  ASSERT_EQ(2U, chain.size());
  EXPECT_EQ(cert_t::from_pem(to_pem(intermediate_cert)), chain[0]);
  EXPECT_EQ(cert_t::from_pem(to_pem(root_cert)), chain[1]);

  /*
  EXPECT_NO_THROW(
    sal::crypto::import_pkcs12(pkcs12, "TestPassword", private_key, chain)
  );
  */
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
cat server.key.pem ca.pem server.pem intermediate.pem \
  | openssl pkcs12 -export -passin pass:ServerPassword -passout pass:TestPassword \
  | openssl base64

Note the order of PEMs, tests expect importing to re-order:
intermediate(s), CA, unrelated
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
  "dAICCAA=";

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
