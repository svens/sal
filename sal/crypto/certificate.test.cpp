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
    chain_as_base64_too_many_certificates,
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


TEST_F(crypto_certificate, import_pkcs12_too_many_certificates) //{{{1
{
  auto pkcs12 = to_der(chain_as_base64_too_many_certificates);

  std::error_code error;
  std::vector<cert_t> chain;
  private_key_t private_key;
  auto cert = sal::crypto::import_pkcs12(pkcs12, "TestPassword",
    private_key, chain, error
  );
  EXPECT_EQ(std::errc::result_out_of_range, error);
  EXPECT_TRUE(!cert);
  EXPECT_TRUE(!private_key);
  EXPECT_TRUE(chain.empty());

  EXPECT_THROW(
    (void)sal::crypto::import_pkcs12(pkcs12, "TestPassword", chain),
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


TEST_F(crypto_certificate, import_pkcs12_valid_no_passphrase) //{{{1
{
#if __sal_os_darwin
  return;
#endif

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
cat server.key.pem intermediate.pem ca.pem server.pem \
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
  "zQICCAA=";

const std::string crypto_certificate::chain_as_base64_pkcs12_no_passphrase = // {{{1
  "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"
  "hgICCAA=";

const std::string crypto_certificate::chain_as_base64_too_many_certificates = // {{{1
  "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"
  "rOHCNl1eQwQIVubFA9ArRywCAggA";

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
