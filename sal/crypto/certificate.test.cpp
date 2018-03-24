#include <sal/crypto/certificate.hpp>
#include <sal/crypto/common.test.hpp>
#include <map>


namespace {

using namespace sal_test;
using namespace std::chrono_literals;

namespace oid = sal::crypto::oid;
using cert_t = sal::crypto::certificate_t;
using private_key_t = sal::crypto::private_key_t;


struct crypto_certificate
  : public sal_test::fixture
{
};


TEST_F(crypto_certificate, ctor) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());
}


TEST_F(crypto_certificate, ctor_copy) //{{{1
{
  auto a = cert_t::from_pem(to_pem(cert::root));
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
  auto a = cert_t::from_pem(to_pem(cert::root));
  EXPECT_FALSE(a.is_null());

  auto b = std::move(a);
  EXPECT_FALSE(b.is_null());
  EXPECT_FALSE(b.serial_number().empty());

  EXPECT_TRUE(a.is_null());
}


TEST_F(crypto_certificate, assign_copy) //{{{1
{
  auto a = cert_t::from_pem(to_pem(cert::root));
  ASSERT_FALSE(a.is_null());

  auto b = cert_t::from_pem(to_pem(cert::intermediate));
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
  auto a = cert_t::from_pem(to_pem(cert::root));
  ASSERT_FALSE(a.is_null());

  auto b = cert_t::from_pem(to_pem(cert::intermediate));
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
  cert_t a, b = cert_t::from_pem(to_pem(cert::root));
  EXPECT_TRUE(a.is_null());
  EXPECT_FALSE(b.is_null());

  a.swap(b);
  EXPECT_FALSE(a.is_null());
  EXPECT_TRUE(b.is_null());
}


TEST_F(crypto_certificate, equals_true) //{{{1
{
  auto a = cert_t::from_pem(to_pem(cert::root)),
       b = cert_t::from_pem(to_pem(cert::root));
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}


TEST_F(crypto_certificate, equals_false) //{{{1
{
  auto a = cert_t::from_pem(to_pem(cert::root)),
       b = cert_t::from_pem(to_pem(cert::leaf));
  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a == b);
}


TEST_F(crypto_certificate, equals_one_null) //{{{1
{
  cert_t a = cert_t::from_pem(to_pem(cert::root)), b;

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
  auto cert = cert_t::from_pem(to_pem(cert::root));
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
    { cert::root,
      { 0x91, 0x02, 0xce, 0x0e, 0xc1, 0x7d, 0x4d, 0xce }

    },
    { cert::intermediate,
      { 0x10, 0x00 }
    },
    { cert::leaf,
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
    { cert::root,
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
    { cert::intermediate,
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
    { cert::leaf,
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
    { cert::root,
      {
        0xcd, 0x81, 0x71, 0xa1, 0xf8, 0x82, 0xf6, 0x04, 0x95, 0x25,
        0x68, 0x81, 0x34, 0x77, 0x2d, 0xa9, 0x5a, 0x1f, 0xc3, 0x9c,
      }
    },
    { cert::intermediate,
      {
        0xcd, 0x81, 0x71, 0xa1, 0xf8, 0x82, 0xf6, 0x04, 0x95, 0x25,
        0x68, 0x81, 0x34, 0x77, 0x2d, 0xa9, 0x5a, 0x1f, 0xc3, 0x9c,
      }
    },
    { cert::leaf,
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
  cert_t cert = cert_t::from_pem(to_pem(cert::without_key_id));
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
    { cert::root,
      {
        0xcd, 0x81, 0x71, 0xa1, 0xf8, 0x82, 0xf6, 0x04, 0x95, 0x25,
        0x68, 0x81, 0x34, 0x77, 0x2d, 0xa9, 0x5a, 0x1f, 0xc3, 0x9c,
      }
    },
    { cert::intermediate,
      {
        0x46, 0x43, 0xee, 0x6f, 0xbe, 0xed, 0x47, 0x01, 0x7d, 0x68,
        0x0c, 0x75, 0x3d, 0xe5, 0x47, 0x7e, 0x82, 0x24, 0xde, 0xb2,
      }
    },
    { cert::leaf,
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
  cert_t cert = cert_t::from_pem(to_pem(cert::without_key_id));
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
    cert::root,
    cert::intermediate,
    cert::leaf,
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
    cert::root,
    cert::intermediate,
    cert::leaf,
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
  auto cert = cert_t::from_pem(to_pem(
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
    "0ZMVcJiK7QldCk/RsD3FL8H5nEs="
  ));
  ASSERT_FALSE(!cert);

  std::error_code error;
  (void)cert.not_after(error);
  EXPECT_TRUE(!error);

  EXPECT_NO_THROW(cert.not_after());
}


TEST_F(crypto_certificate, not_expired) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(cert::root));
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
  auto cert = cert_t::from_pem(to_pem(cert::root));
  ASSERT_FALSE(!cert);

  // 30 years in past
  auto past = sal::now() - 30 * 365 * 24h;
  EXPECT_FALSE(cert.not_expired(past));
  EXPECT_FALSE(cert.not_expired(365 * 24h, past));
}


TEST_F(crypto_certificate, not_expired_future) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(cert::root));
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
    { cert::root, cert::root },
    { cert::intermediate, cert::root },
    { cert::leaf, cert::intermediate },
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
    cert::root,
    cert::intermediate,
    cert::leaf,
  };

  auto issuer_cert = cert_t::from_pem(to_pem(cert::leaf));
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
  cert_t this_cert, issuer_cert = cert_t::from_pem(to_pem(cert::root));
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
  cert_t this_cert = cert_t::from_pem(to_pem(cert::root)), issuer_cert;
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
    { cert::root, true },
    { cert::intermediate, false },
    { cert::leaf, false },
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
    { cert::root,
      {
        { oid::country_name, "EE" },
        { oid::state_or_province_name, "Estonia" },
        { oid::organization_name, "SAL" },
        { oid::organizational_unit_name, "SAL CA" },
        { oid::common_name, "SAL Root CA" },
      }
    },
    { cert::intermediate,
      {
        { oid::country_name, "EE" },
        { oid::state_or_province_name, "Estonia" },
        { oid::organization_name, "SAL" },
        { oid::organizational_unit_name, "SAL CA" },
        { oid::common_name, "SAL Root CA" },
      }
    },
    { cert::leaf,
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
    { cert::root, { { oid::common_name, "SAL Root CA" }, } },
    { cert::intermediate, { { oid::common_name, "SAL Root CA" }, } },
    { cert::leaf, { { oid::common_name, "SAL Intermediate CA" }, } },
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
  auto cert = cert_t::from_pem(to_pem(cert::root));
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
  auto cert = cert_t::from_pem(to_pem(cert::root));
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
    { cert::root,
      {
        { oid::country_name, "EE" },
        { oid::state_or_province_name, "Estonia" },
        { oid::organization_name, "SAL" },
        { oid::organizational_unit_name, "SAL CA" },
        { oid::common_name, "SAL Root CA" },
      }
    },
    { cert::intermediate,
      {
        { oid::country_name, "EE" },
        { oid::state_or_province_name, "Estonia" },
        { oid::organization_name, "SAL" },
        { oid::organizational_unit_name, "SAL CA" },
        { oid::common_name, "SAL Intermediate CA" },
      }
    },
    { cert::leaf,
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
    { cert::root, { { oid::common_name, "SAL Root CA" }, } },
    { cert::intermediate, { { oid::common_name, "SAL Intermediate CA" }, } },
    { cert::leaf, { { oid::common_name, "test.sal.ee" }, } },
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
  auto cert = cert_t::from_pem(to_pem(cert::root));
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
  auto cert = cert_t::from_pem(to_pem(cert::root));
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
    { cert::root,
      { }
    },
    { cert::intermediate,
      { }
    },
    { cert::leaf,
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
    { cert::root,
      { }
    },
    { cert::intermediate,
      { }
    },
    { cert::leaf,
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
    { cert::root,
      { }
    },
    { cert::intermediate,
      { }
    },
    { cert::leaf,
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
  auto cert = cert_t::from_pem(to_pem(cert::root));

  uint8_t data[8192];
  std::error_code error;
  auto data_end = cert.to_der(data, error);
  ASSERT_TRUE(!error);
  ASSERT_NE(nullptr, data_end);
  std::vector<uint8_t> der(data, data_end);

  auto expected_der = to_der(cert::root);
  EXPECT_EQ(expected_der, der);

  EXPECT_NO_THROW(
    (void)cert.to_der(data)
  );
}


TEST_F(crypto_certificate, to_der_from_null) //{{{1
{
  cert_t cert;
  ASSERT_TRUE(cert.is_null());

  uint8_t data[8192];
  std::error_code error;
  EXPECT_EQ(nullptr, cert.to_der(data, error));
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.to_der(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, to_der_result_exact_range) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(cert::root));
  ASSERT_FALSE(cert.is_null());

  uint8_t data[8192];
  std::error_code error;
  auto data_end = cert.to_der(data, error);
  ASSERT_TRUE(!error);
  ASSERT_NE(nullptr, data_end);

  std::vector<uint8_t> der(data_end - data);
  auto end = cert.to_der(der, error);
  EXPECT_TRUE(!error);
  EXPECT_EQ(end, der.data() + der.size());
  EXPECT_EQ(size_t(data_end - data), der.size());
}


TEST_F(crypto_certificate, to_der_result_out_of_range) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(cert::root));
  ASSERT_FALSE(cert.is_null());

  uint8_t data[1];
  std::error_code error;
  EXPECT_EQ(nullptr, cert.to_der(data, error));
  EXPECT_EQ(std::errc::result_out_of_range, error);

  EXPECT_THROW(
    (void)cert.to_der(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, to_der_empty_vector)
{
  auto cert = cert_t::from_pem(to_pem(cert::root));
  ASSERT_FALSE(cert.is_null());

  std::vector<uint8_t> data;
  std::error_code error;
  EXPECT_EQ(nullptr, cert.to_der(data, error));
  EXPECT_EQ(std::errc::result_out_of_range, error);

  EXPECT_THROW(
    (void)cert.to_der(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, to_der_vector) //{{{1
{
  auto cert = cert_t::from_pem(to_pem(cert::root));

  std::error_code error;
  auto der = cert.to_der(error);
  ASSERT_TRUE(!error);

  auto expected_der = to_der(cert::root);
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
  auto data = to_der(cert::root);

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
  auto data = to_der(cert::root);
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
  auto data = to_der(cert::root);
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
  auto data = cert::root;

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
  auto data = cert::root;
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
  auto data = to_pem(cert::root);
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
  auto data = to_pem(cert::root);
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
  auto data = to_pem(cert::root + 'X');

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
  auto raw_data = to_der(cert::root);
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
  auto pkcs12 = to_der(cert::pkcs12);

  std::error_code error;
  private_key_t private_key;
  auto chain = sal::crypto::import_pkcs12(pkcs12,
    "TestPassword",
    &private_key,
    error
  );
  ASSERT_TRUE(!error) << error.message();

  ASSERT_EQ(3U, chain.size());
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::leaf)), chain[0]);
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::intermediate)), chain[1]);
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::root)), chain[2]);

  EXPECT_NO_THROW(
    sal::crypto::import_pkcs12(pkcs12, "TestPassword", &private_key)
  );
}


TEST_F(crypto_certificate, import_pkcs12_without_private_key) //{{{1
{
  auto pkcs12 = to_der(cert::pkcs12);

  std::error_code error;
  auto chain = sal::crypto::import_pkcs12(pkcs12, "TestPassword", error);
  ASSERT_TRUE(!error) << error.message();

  ASSERT_EQ(3U, chain.size());
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::leaf)), chain[0]);
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::intermediate)), chain[1]);
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::root)), chain[2]);

  EXPECT_NO_THROW(
    sal::crypto::import_pkcs12(pkcs12, "TestPassword")
  );
}


TEST_F(crypto_certificate, import_pkcs12_no_data) //{{{1
{
  std::vector<uint8_t> pkcs12;

  std::error_code error;
  auto chain = sal::crypto::import_pkcs12(pkcs12, "TestPassword", error);
  EXPECT_TRUE(chain.empty());

  ASSERT_FALSE(!error);
  EXPECT_NE(nullptr, error.category().name());
  EXPECT_FALSE(error.message().empty());

  EXPECT_THROW(
    (void)sal::crypto::import_pkcs12(pkcs12, "TestPassword"),
    std::system_error
  );
}


TEST_F(crypto_certificate, import_pkcs12_partial_data) //{{{1
{
  auto pkcs12 = to_der(cert::pkcs12);
  pkcs12.resize(pkcs12.size() / 2);

  std::error_code error;
  auto chain = sal::crypto::import_pkcs12(pkcs12, "TestPassword", error);
  EXPECT_FALSE(!error);
  EXPECT_TRUE(chain.empty());

  EXPECT_THROW(
    (void)sal::crypto::import_pkcs12(pkcs12, "TestPassword"),
    std::system_error
  );
}


TEST_F(crypto_certificate, import_pkcs12_invalid_data) //{{{1
{
  auto pkcs12 = to_der(cert::pkcs12);
  for (auto &b: pkcs12)
  {
    b ^= 1;
  }

  std::error_code error;
  auto chain = sal::crypto::import_pkcs12(pkcs12, "TestPassword", error);
  EXPECT_FALSE(!error);
  EXPECT_TRUE(chain.empty());

  EXPECT_THROW(
    (void)sal::crypto::import_pkcs12(pkcs12, "TestPassword"),
    std::system_error
  );
}


TEST_F(crypto_certificate, import_pkcs12_no_passphrase) //{{{1
{
  auto pkcs12 = to_der(cert::pkcs12);

  std::error_code error;
  auto chain = sal::crypto::import_pkcs12(pkcs12, "", error);
  EXPECT_FALSE(!error);
  EXPECT_TRUE(chain.empty());

  EXPECT_THROW(
    (void)sal::crypto::import_pkcs12(pkcs12, ""),
    std::system_error
  );
}


TEST_F(crypto_certificate, import_pkcs12_valid_no_passphrase) //{{{1
{
#if __sal_os_macos
  // MacOS refuses to import PKCS12 with no passphrase
  return;
#endif

  auto pkcs12 = to_der(cert::pkcs12_no_passphrase);

  std::error_code error;
  private_key_t private_key;
  auto chain = sal::crypto::import_pkcs12(pkcs12, "", &private_key, error);
  ASSERT_TRUE(!error) << error.message();

  EXPECT_FALSE(!private_key);
  ASSERT_EQ(3U, chain.size());
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::leaf)), chain[0]);
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::intermediate)), chain[1]);
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::root)), chain[2]);

  EXPECT_NO_THROW(
    sal::crypto::import_pkcs12(pkcs12, "", &private_key)
  );
}


TEST_F(crypto_certificate, import_pkcs12_invalid_passphrase) //{{{1
{
  auto pkcs12 = to_der(cert::pkcs12);

  std::error_code error;
  auto chain = sal::crypto::import_pkcs12(pkcs12, case_name, error);
  EXPECT_FALSE(!error);
  EXPECT_TRUE(chain.empty());

  EXPECT_THROW(
    (void)sal::crypto::import_pkcs12(pkcs12, case_name),
    std::system_error
  );
}


TEST_F(crypto_certificate, ostream) //{{{1
{
  const std::pair<std::string, std::string> certs[] =
  {
    {
      cert::root,
      "C=EE; ST=Estonia; O=SAL; OU=SAL CA; CN=SAL Root CA"
    },
    {
      cert::intermediate,
      "C=EE; ST=Estonia; O=SAL; OU=SAL CA; CN=SAL Intermediate CA"
    },
    {
      cert::leaf,
      "C=EE; ST=Estonia; O=SAL; OU=SAL Test; CN=test.sal.ee"
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(!cert);

    std::ostringstream oss;
    oss << cert;
    EXPECT_EQ(cert_pem.second, oss.str());
  }
}


TEST_F(crypto_certificate, ostream_issuer) //{{{1
{
  const std::pair<std::string, std::string> certs[] =
  {
    {
      cert::root,
      "C=EE; ST=Estonia; O=SAL; OU=SAL CA; CN=SAL Root CA"
    },
    {
      cert::intermediate,
      "C=EE; ST=Estonia; O=SAL; OU=SAL CA; CN=SAL Root CA"
    },
    {
      cert::leaf,
      "C=EE; ST=Estonia; O=SAL; OU=SAL CA; CN=SAL Intermediate CA"
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(!cert);

    std::ostringstream oss;
    oss << cert.format(cert.issuer());
    EXPECT_EQ(cert_pem.second, oss.str());
  }
}


TEST_F(crypto_certificate, ostream_subject) //{{{1
{
  const std::pair<std::string, std::string> certs[] =
  {
    {
      cert::root,
      "C=EE; ST=Estonia; O=SAL; OU=SAL CA; CN=SAL Root CA"
    },
    {
      cert::intermediate,
      "C=EE; ST=Estonia; O=SAL; OU=SAL CA; CN=SAL Intermediate CA"
    },
    {
      cert::leaf,
      "C=EE; ST=Estonia; O=SAL; OU=SAL Test; CN=test.sal.ee"
    },
  };

  for (const auto &cert_pem: certs)
  {
    auto cert = cert_t::from_pem(to_pem(cert_pem.first));
    ASSERT_FALSE(!cert);

    std::ostringstream oss;
    oss << cert.format(cert.subject());
    EXPECT_EQ(cert_pem.second, oss.str());
  }
}


TEST_F(crypto_certificate, ostream_escape) //{{{1
{
  static const auto cert = cert_t::from_pem(to_pem(
    "MIIBkzCCAT2gAwIBAgIJAJS/4Y1RBSIaMA0GCSqGSIb3DQEBBQUAMBMxETAPBgNV"
    "BAMUCEEsKzw+IztCMB4XDTE3MDkxNDE1MDU0OFoXDTE3MTAxNDE1MDU0OFowEzER"
    "MA8GA1UEAxQIQSwrPD4jO0IwXDANBgkqhkiG9w0BAQEFAANLADBIAkEAywCPrX3d"
    "VJE9f3uzGH5WbtpeQnRKfmAe8YEqk+6d9aUZ2NrgCF6vYEcs32ivdf25SVLVsgcy"
    "pf+B5BzdUyZw0QIDAQABo3QwcjAdBgNVHQ4EFgQUP9SvXUSWIRTTsHJ8a6vYGTLj"
    "sRgwQwYDVR0jBDwwOoAUP9SvXUSWIRTTsHJ8a6vYGTLjsRihF6QVMBMxETAPBgNV"
    "BAMUCEEsKzw+IztCggkAlL/hjVEFIhowDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0B"
    "AQUFAANBAApsD+cPytGVZFv08arhfXLSDamfm6fq86aCytB53q6jn+IdjB7e2mpm"
    "mp7oULFgeYNE6+bk2PMqkon3zh0tR78="
  ));
  ASSERT_FALSE(!cert);

  std::ostringstream oss;
  oss << cert;
  EXPECT_EQ("CN=A\\,\\+\\<\\>\\#\\;B", oss.str());
}


auto all = [](const sal::crypto::certificate_t &)
{
  return true;
};

auto none = [](const sal::crypto::certificate_t &)
{
  return false;
};


TEST_F(crypto_certificate, load_first) //{{{1
{
  std::error_code error;
  sal::crypto::certificate_t::load_first(all, error);
  EXPECT_TRUE(!error);

  EXPECT_NO_THROW(
    sal::crypto::certificate_t::load_first(all)
  );
}


TEST_F(crypto_certificate, load_first_fail) //{{{1
{
  std::error_code error;
  sal::crypto::certificate_t::load_first(none, error);
  EXPECT_EQ(std::errc::no_such_file_or_directory, error);

  EXPECT_THROW(
    sal::crypto::certificate_t::load_first(none),
    std::system_error
  );
}


TEST_F(crypto_certificate, load) //{{{1
{
  std::error_code error;
  auto certs = sal::crypto::certificate_t::load(all, error);
  EXPECT_TRUE(!error) << error.message();
  EXPECT_FALSE(certs.empty());

  EXPECT_NO_THROW(
    sal::crypto::certificate_t::load(all)
  );
}


TEST_F(crypto_certificate, load_fail) //{{{1
{
  std::error_code error;
  auto certs = sal::crypto::certificate_t::load(none, error);
  EXPECT_TRUE(!error);
  EXPECT_TRUE(certs.empty());

  EXPECT_NO_THROW(
    sal::crypto::certificate_t::load(none)
  );
}


TEST_F(crypto_certificate, load_with_common_name) //{{{1
{
  auto certs = sal::crypto::certificate_t::load(
    sal::crypto::with_common_name("test.sal.ee")
  );
  ASSERT_EQ(1U, certs.size());
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::leaf)), certs[0]);
}


TEST_F(crypto_certificate, load_with_common_name_fail) //{{{1
{
  auto certs = sal::crypto::certificate_t::load(
    sal::crypto::with_common_name(case_name)
  );
  EXPECT_TRUE(certs.empty());
}


TEST_F(crypto_certificate, load_with_subject_alt_name_fqdn) //{{{1
{
  auto certs = sal::crypto::certificate_t::load(
    sal::crypto::with_fqdn("sal.alt.ee")
  );
  ASSERT_EQ(1U, certs.size());
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::leaf)), certs[0]);
}


TEST_F(crypto_certificate, load_with_subject_alt_name_fqdn_fail) //{{{1
{
  auto certs = sal::crypto::certificate_t::load(
    sal::crypto::with_fqdn(case_name)
  );
  EXPECT_TRUE(certs.empty());
}


TEST_F(crypto_certificate, load_with_subject_alt_name_wildcard_fqdn) //{{{1
{
  auto certs = sal::crypto::certificate_t::load(
    sal::crypto::with_fqdn("success.sal.alt.ee")
  );
  ASSERT_EQ(1U, certs.size());
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::leaf)), certs[0]);
}


TEST_F(crypto_certificate, load_with_subject_alt_name_wildcard_fqdn_fail) //{{{1
{
  auto certs = sal::crypto::certificate_t::load(
    sal::crypto::with_fqdn("fail-sal.alt.ee")
  );
  EXPECT_TRUE(certs.empty());
}


TEST_F(crypto_certificate, load_with_sha1_thumbprint) //{{{1
{
  auto certs = sal::crypto::certificate_t::load(
    sal::crypto::with_sha1_thumbprint({
      0xef, 0xbe, 0x01, 0xb6, 0x43, 0x34, 0x57, 0xae, 0xf9, 0xfc,
      0x66, 0x06, 0x4d, 0xe2, 0x09, 0x50, 0xee, 0xb4, 0x10, 0x40,
    })
  );
  ASSERT_EQ(1U, certs.size());
  EXPECT_EQ(cert_t::from_pem(to_pem(cert::leaf)), certs[0]);
}


TEST_F(crypto_certificate, load_with_sha1_thumbprint_fail) //{{{1
{
  auto certs = sal::crypto::certificate_t::load(
    sal::crypto::with_sha1_thumbprint({
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    })
  );
  EXPECT_TRUE(certs.empty());
}


//}}}1


} // namespace
