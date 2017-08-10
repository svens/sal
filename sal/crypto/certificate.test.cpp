#include <sal/crypto/certificate.hpp>
#include <sal/buf_ptr.hpp>
#include <sal/encode.hpp>
#include <sal/common.test.hpp>
#include <map>


namespace {


using namespace std::chrono_literals;

namespace oid = sal::crypto::oid;
using cert_t = sal::crypto::certificate_t;


struct crypto_certificate
  : public sal_test::fixture
{
  static const std::string
    root_cert,
    intermediate_cert,
    leaf_cert,
    leaf_cert_without_key_id;

  static std::vector<uint8_t> to_der (const std::string &base64)
  {
    return sal::decode<sal::base64>(base64);
  }

  static std::string to_armored_pem (const std::string &base64,
    bool with_suffix = true)
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
    if (with_suffix)
    {
      result += "\n-----END CERTIFICATE-----\n";
    }
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
  auto a = cert_t::from_pem(root_cert);
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
  auto a = cert_t::from_pem(root_cert);
  EXPECT_FALSE(a.is_null());

  auto b = std::move(a);
  EXPECT_FALSE(b.is_null());
  EXPECT_FALSE(b.serial_number().empty());

  EXPECT_TRUE(a.is_null());
}


TEST_F(crypto_certificate, assign_copy) //{{{1
{
  auto a = cert_t::from_pem(root_cert);
  ASSERT_FALSE(a.is_null());

  auto b = cert_t::from_pem(intermediate_cert);
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
  auto a = cert_t::from_pem(root_cert);
  ASSERT_FALSE(a.is_null());

  auto b = cert_t::from_pem(intermediate_cert);
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
  cert_t a, b = cert_t::from_pem(root_cert);
  EXPECT_TRUE(a.is_null());
  EXPECT_FALSE(b.is_null());

  a.swap(b);
  EXPECT_FALSE(a.is_null());
  EXPECT_TRUE(b.is_null());
}


TEST_F(crypto_certificate, version) //{{{1
{
  auto cert = cert_t::from_pem(root_cert);
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
    auto cert = cert_t::from_pem(cert_pem.first);
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
    auto cert = cert_t::from_pem(cert_pem.first);
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
  cert_t cert = cert_t::from_pem(leaf_cert_without_key_id);
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
    auto cert = cert_t::from_pem(cert_pem.first);
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
  cert_t cert = cert_t::from_pem(leaf_cert_without_key_id);
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

  for (const auto &pem: certs)
  {
    auto cert = cert_t::from_pem(pem);
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

  for (const auto &pem: certs)
  {
    auto cert = cert_t::from_pem(pem);
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


TEST_F(crypto_certificate, not_expired) //{{{1
{
  auto cert = cert_t::from_pem(root_cert);
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
  auto cert = cert_t::from_pem(root_cert);
  ASSERT_FALSE(!cert);

  // 30 years in past
  auto past = sal::now() - 30 * 365 * 24h;
  EXPECT_FALSE(cert.not_expired(past));
  EXPECT_FALSE(cert.not_expired(365 * 24h, past));
}


TEST_F(crypto_certificate, not_expired_future) //{{{1
{
  auto cert = cert_t::from_pem(root_cert);
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
    auto this_cert = cert_t::from_pem(pair.first);
    ASSERT_FALSE(this_cert.is_null());

    auto issuer_cert = cert_t::from_pem(pair.second);
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

  for (const auto &cert_pem: certs)
  {
    auto this_cert = cert_t::from_pem(cert_pem);
    ASSERT_FALSE(this_cert.is_null());

    auto issuer_cert = cert_t::from_pem(leaf_cert);
    ASSERT_FALSE(issuer_cert.is_null());

    std::error_code error;
    EXPECT_FALSE(this_cert.issued_by(issuer_cert, error));
    EXPECT_TRUE(!error);

    EXPECT_NO_THROW((void)this_cert.issued_by(issuer_cert));
  }
}


TEST_F(crypto_certificate, issued_by_null_cert) //{{{1
{
  cert_t this_cert, issuer_cert = cert_t::from_pem(root_cert);
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
  cert_t this_cert = cert_t::from_pem(root_cert), issuer_cert;
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
    auto cert = cert_t::from_pem(cert_pem.first);
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
    auto cert = cert_t::from_pem(cert_pem.first);
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
    auto cert = cert_t::from_pem(cert_pem.first);
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
  auto cert = cert_t::from_pem(root_cert);
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
  auto cert = cert_t::from_pem(root_cert);
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
    auto cert = cert_t::from_pem(cert_pem.first);
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
    auto cert = cert_t::from_pem(cert_pem.first);
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
  auto cert = cert_t::from_pem(root_cert);
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
  auto cert = cert_t::from_pem(root_cert);
  ASSERT_FALSE(cert.is_null());

  std::error_code error;
  auto subject = cert.subject(case_name, error);
  ASSERT_TRUE(!error);
  EXPECT_TRUE(subject.empty());

  EXPECT_NO_THROW(
    EXPECT_TRUE(cert.subject(case_name).empty())
  );
}


TEST_F(crypto_certificate, issuer_alt_name) //{{{1
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
    auto cert = cert_t::from_pem(cert_pem.first);
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    auto alt_name = cert.issuer_alt_name(error);
    ASSERT_TRUE(!error);
    EXPECT_EQ(cert_pem.second, alt_name);

    EXPECT_NO_THROW((void)cert.issuer_alt_name());
  }
}


TEST_F(crypto_certificate, issuer_alt_name_from_null) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());

  std::error_code error;
  EXPECT_TRUE(cert.issuer_alt_name(error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.issuer_alt_name(),
    std::system_error
  );
}


TEST_F(crypto_certificate, subject_alt_name) //{{{1
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
    auto cert = cert_t::from_pem(cert_pem.first);
    ASSERT_FALSE(cert.is_null());

    std::error_code error;
    auto alt_name = cert.subject_alt_name(error);
    ASSERT_TRUE(!error);
    EXPECT_EQ(cert_pem.second, alt_name);

    EXPECT_NO_THROW((void)cert.subject_alt_name());
  }
}


TEST_F(crypto_certificate, subject_alt_name_from_null) //{{{1
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());

  std::error_code error;
  EXPECT_TRUE(cert.subject_alt_name(error).empty());
  EXPECT_EQ(std::errc::bad_address, error);

  EXPECT_THROW(
    (void)cert.subject_alt_name(),
    std::system_error
  );
}


TEST_F(crypto_certificate, to_der) //{{{1
{
  auto cert = cert_t::from_pem(root_cert);

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
  auto cert = cert_t::from_pem(root_cert);
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
  auto cert = cert_t::from_pem(root_cert);
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
  auto cert = cert_t::from_pem(root_cert);

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
  ASSERT_EQ(std::errc::invalid_argument, error);
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
  ASSERT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
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
  ASSERT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
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
  auto cert = cert_t::from_pem(data, error);
  ASSERT_TRUE(!error) << error.message() << " (" << error.value() << ')';
  EXPECT_FALSE(cert.is_null());

  EXPECT_NO_THROW(cert_t::from_pem(data));
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

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_invalid_data) //{{{1
{
  auto data = root_cert;
  data[0] = 'X';

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_with_armor) //{{{1
{
  auto data = to_armored_pem(root_cert);

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_TRUE(!error) << error.message() << " (" << error.value() << ')';
  EXPECT_FALSE(cert.is_null());

  EXPECT_NO_THROW(cert_t::from_pem(data));
}


TEST_F(crypto_certificate, from_pem_with_armor_empty_data) //{{{1
{
  std::string data;
  data = to_armored_pem(data);

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::invalid_argument, error);
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_with_armor_insufficient_data) //{{{1
{
  auto data = root_cert;
  data.resize(data.size() / 2);
  data = to_armored_pem(data);

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_with_armor_invalid_data) //{{{1
{
  auto data = root_cert;
  data[0] = 'X';
  data = to_armored_pem(data);

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_with_armor_invalid_data_length) //{{{1
{
  auto data = 'A' + root_cert;
  data = to_armored_pem(data);

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::message_size, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


TEST_F(crypto_certificate, from_pem_with_armor_invalid_prefix) //{{{1
{
  auto data = case_name + to_armored_pem(root_cert);

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_TRUE(!error) << error.message();
  EXPECT_FALSE(cert.is_null());

  EXPECT_NO_THROW(
    cert_t::from_pem(data)
  );
}


TEST_F(crypto_certificate, from_pem_with_armor_invalid_suffix) //{{{1
{
  auto data = to_armored_pem(root_cert) + case_name;

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_TRUE(!error) << error.message();
  EXPECT_FALSE(cert.is_null());

  EXPECT_NO_THROW(
    cert_t::from_pem(data)
  );
}


TEST_F(crypto_certificate, from_pem_with_armor_missing_suffix) //{{{1
{
  auto data = to_armored_pem(root_cert, false);

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_TRUE(!error) << error.message();
  EXPECT_FALSE(cert.is_null());

  EXPECT_NO_THROW(
    cert_t::from_pem(data)
  );
}


TEST_F(crypto_certificate, from_pem_with_too_big_data) //{{{1
{
  auto data = root_cert;

  // use internal knowledge here: implementation uses 8kB buffer, let's create
  // here bigger data (remove padding, multiply data, restore padding)
  while (data.back() == '=')
  {
    data.pop_back();
  }
  while (data.size() < 32 * 1024)
  {
    data = data + data;
  }
  while (data.size() % 4 != 0)
  {
    data.push_back('=');
  }

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_EQ(std::errc::no_buffer_space, error) << error.message();
  EXPECT_TRUE(cert.is_null());

  EXPECT_THROW(
    cert_t::from_pem(data),
    std::system_error
  );
}


//}}}1

// see scripts/make_ca.sh
// Not Before: Aug  7 17:26:xx 2017 GMT
// Not After : Jul  3 17:26:xx 2037 GMT

const std::string crypto_certificate::root_cert = // {{{1
  "MIIFjjCCA3agAwIBAgIJAJECzg7BfU3OMA0GCSqGSIb3DQEBCwUAMFQxCzAJBgNV"
  "BAYTAkVFMRAwDgYDVQQIDAdFc3RvbmlhMQwwCgYDVQQKDANTQUwxDzANBgNVBAsM"
  "BlNBTCBDQTEUMBIGA1UEAwwLU0FMIFJvb3QgQ0EwHhcNMTcwODA3MTcyNjQyWhcN"
  "MzcwODAyMTcyNjQyWjBUMQswCQYDVQQGEwJFRTEQMA4GA1UECAwHRXN0b25pYTEM"
  "MAoGA1UECgwDU0FMMQ8wDQYDVQQLDAZTQUwgQ0ExFDASBgNVBAMMC1NBTCBSb290"
  "IENBMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA9EyXF65IyFwX/E4u"
  "IV1njWpWcat3kwByJG/rf4H5ADelksLQ72te4vYlbb3fgaLyFyOneLOVUz9W7tFT"
  "hTReCCPB0PJR77QbXMZyOsleK9NGvf4B5IU0F64UOxoFvYlv0xdSocIC3ut1/be5"
  "YrMht8vlAx3gJdczIpMCoRgQp+YtMPANfwlC8VWhJK8LULOAEyfrjoW6dpT7rhAE"
  "P5cZFpC22niKOSIot8sR8+Td/yXQcqv2dnKELpTngqPT2+Db5eJVPXFniWyIvG1p"
  "ppToikh0YtvPEAcxdGA9QR0uJnpqrkQIm5PeB3OxTGYTXsNsSj80IuIQuMPIVtSK"
  "kqwsO6OiCIhcDvNQ2Atn7ahP3CqX8zn+ez29DraF46f5win6rPfPF/W5B/SGfis3"
  "lCLkbc483nB7D0KTuoPWOdvfvESiUiMtFyTzztxDDeAjibTUujGH+qEZDR/0l7X/"
  "DtSZVuHuc+SIPjYR07beyIw/2qNUp34CNkRyh8XcFSEZUOZMZY2+/snvdDnNXB+a"
  "XiDVchu4yWGHxvcxbWpWAQkJpm1FHIrh4dJtuWKPE9eqGbpBSgxDB3z9drpBbnDU"
  "qfioq1H2hemM5aCQZw69ajzCrEtjFzj/npIMZ3yNQyzyejGP7z7C8WwGj9idOfnT"
  "Rl/X1CJzWlTJiE+4omhXpA4Mwc8CAwEAAaNjMGEwHQYDVR0OBBYEFM2BcaH4gvYE"
  "lSVogTR3LalaH8OcMB8GA1UdIwQYMBaAFM2BcaH4gvYElSVogTR3LalaH8OcMA8G"
  "A1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgGGMA0GCSqGSIb3DQEBCwUAA4IC"
  "AQCXar8SXYk9aNSf971Jde2Q+hpOLMaY5CP/PePFarIRgw+3u4HBjYrsXp2nnZsT"
  "L1W6l92zWmGYnUa7pJCYXLX693zcfcqAfWRJD8jUhSFLOKDP/O0D2CzB7X5uJTBf"
  "KV5KnEeLjGVeuuT8D4gZVJaFcvC5BrnU/rWSxQitX1QDVMTGzmIPzP9KO26VPTuh"
  "Qm1OmXqX+P0gLrnpW54+Bt2Kb/1NTct6WMEAr4HDTc3PniiiWYVjadUBIC/45xmc"
  "KAXfqe4dvmcHC6DTOipGTNNK06rsSglqcjrmDwPXbxfqBRLOBK+llsjjElLRendS"
  "fwqNl2l5GBbdCexEbQOOo5PjK77HKdsRAHeo3d8ZvOUd20Zw/IUcU0Rdi6DO5Fui"
  "8xL8YCSRrtcTzM52oo7yb8lDEtrNLoC/uV6ZBtfu1g6JR6+7PrsgDsjQi1O6ewXm"
  "HVMYNFFoQOKL3CZD2b+6j7UTr2sbeaGVm84TM9aBbpLpl+tpBEv2Wr9zzYHSWVz6"
  "4ronLfkTatZ17PW8zzBrjRoeQXDVoE2uik3ip9sH02rOEOJLo+TVHQIq4SINN5s5"
  "JZr88Qy2tDeD7P2ASVRe7ss8G4nyhk8aivrKGSdlm6eUqWvGUPCa2RrIpleqwMwH"
  "17VXMW1G1Disvw3DfjH4ZFYCH1jHXBZvxXfHt003AiCQaQ==";

const std::string crypto_certificate::intermediate_cert = // {{{1
  "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";

const std::string crypto_certificate::leaf_cert = // {{{1
  "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"
  "QOXpu3kF+2btU7ro9vJPcWs=";

const std::string crypto_certificate::leaf_cert_without_key_id = // {{{1
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

// }}}

} // namespace
