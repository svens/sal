#include <sal/crypto/certificate.hpp>
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


TEST_F(crypto_certificate, ctor)
{
  cert_t cert;
  EXPECT_TRUE(cert.is_null());
}


TEST_F(crypto_certificate, ctor_copy)
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


TEST_F(crypto_certificate, ctor_move)
{
  auto a = cert_t::from_pem(root_cert);
  EXPECT_FALSE(a.is_null());

  auto b = std::move(a);
  EXPECT_FALSE(b.is_null());
  EXPECT_FALSE(b.serial_number().empty());

  EXPECT_TRUE(a.is_null());
}


TEST_F(crypto_certificate, assign_copy)
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


TEST_F(crypto_certificate, assign_move)
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


TEST_F(crypto_certificate, swap)
{
  cert_t a, b = cert_t::from_pem(root_cert);
  EXPECT_TRUE(a.is_null());
  EXPECT_FALSE(b.is_null());

  a.swap(b);
  EXPECT_FALSE(a.is_null());
  EXPECT_TRUE(b.is_null());
}


TEST_F(crypto_certificate, version)
{
  auto cert = cert_t::from_pem(root_cert);
  ASSERT_FALSE(!cert);
  EXPECT_EQ(3, cert.version());
}


TEST_F(crypto_certificate, version_from_null)
{
  cert_t cert;
  ASSERT_TRUE(!cert);
  EXPECT_EQ(0, cert.version());
}


TEST_F(crypto_certificate, serial_number)
{
  const std::pair<std::string, std::vector<uint8_t>> certs[] =
  {
    { root_cert,
      { 0x93, 0x3a, 0xca, 0x86, 0x76, 0xa6, 0x4c, 0xd6 }
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


TEST_F(crypto_certificate, serial_number_from_null)
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


TEST_F(crypto_certificate, authority_key_identifier)
{
  const std::pair<std::string, std::vector<uint8_t>> certs[] =
  {
    { root_cert,
      {
        0xd8, 0x59, 0x5f, 0xcf, 0x86, 0x9c, 0xcb, 0x52, 0x29, 0x98,
        0x5f, 0x55, 0xf6, 0x0e, 0xe5, 0x8e, 0xaa, 0x24, 0x82, 0xe1,
      }
    },
    { intermediate_cert,
      {
        0xd8, 0x59, 0x5f, 0xcf, 0x86, 0x9c, 0xcb, 0x52, 0x29, 0x98,
        0x5f, 0x55, 0xf6, 0x0e, 0xe5, 0x8e, 0xaa, 0x24, 0x82, 0xe1,
      }
    },
    { leaf_cert,
      {
        0x9a, 0x34, 0xc2, 0x55, 0x79, 0xbc, 0xda, 0xbc, 0x12, 0x54,
        0x43, 0x36, 0xb5, 0x8d, 0x1e, 0x7b, 0x16, 0xbf, 0xd2, 0x63,
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


TEST_F(crypto_certificate, authority_key_identifier_from_null)
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


TEST_F(crypto_certificate, authority_key_identifier_none)
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


TEST_F(crypto_certificate, subject_key_identifier)
{
  const std::pair<std::string, std::vector<uint8_t>> certs[] =
  {
    { root_cert,
      {
        0xd8, 0x59, 0x5f, 0xcf, 0x86, 0x9c, 0xcb, 0x52, 0x29, 0x98,
        0x5f, 0x55, 0xf6, 0x0e, 0xe5, 0x8e, 0xaa, 0x24, 0x82, 0xe1,
      }
    },
    { intermediate_cert,
      {
        0x9a, 0x34, 0xc2, 0x55, 0x79, 0xbc, 0xda, 0xbc, 0x12, 0x54,
        0x43, 0x36, 0xb5, 0x8d, 0x1e, 0x7b, 0x16, 0xbf, 0xd2, 0x63,
      }
    },
    { leaf_cert,
      {
        0x11, 0xac, 0xe4, 0x02, 0xf6, 0x74, 0x0d, 0xa4, 0x0a, 0x0e,
        0x9d, 0xa9, 0x96, 0x51, 0x3d, 0x55, 0x6c, 0xff, 0x73, 0xdc,
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


TEST_F(crypto_certificate, subject_key_identifier_from_null)
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


TEST_F(crypto_certificate, subject_key_identifier_none)
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


TEST_F(crypto_certificate, not_before)
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


TEST_F(crypto_certificate, not_before_null)
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


TEST_F(crypto_certificate, not_after)
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


TEST_F(crypto_certificate, not_after_null)
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


TEST_F(crypto_certificate, issued_by)
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


TEST_F(crypto_certificate, issued_by_leaf)
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


TEST_F(crypto_certificate, issued_by_null_cert)
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


TEST_F(crypto_certificate, issued_by_null_issuer)
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


TEST_F(crypto_certificate, is_self_signed)
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


TEST_F(crypto_certificate, is_self_signed_from_null)
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


TEST_F(crypto_certificate, issuer)
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


TEST_F(crypto_certificate, issuer_from_null)
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


TEST_F(crypto_certificate, issuer_with_oid)
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


TEST_F(crypto_certificate, issuer_with_oid_from_null)
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


TEST_F(crypto_certificate, issuer_with_oid_missing)
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


TEST_F(crypto_certificate, issuer_with_oid_invalid)
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


TEST_F(crypto_certificate, subject)
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


TEST_F(crypto_certificate, subject_from_null)
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


TEST_F(crypto_certificate, subject_with_oid)
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


TEST_F(crypto_certificate, subject_with_oid_from_null)
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


TEST_F(crypto_certificate, subject_with_oid_missing)
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


TEST_F(crypto_certificate, subject_with_oid_invalid)
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


TEST_F(crypto_certificate, from_der)
{
  auto data = to_der(root_cert);

  std::error_code error;
  auto cert = cert_t::from_der(data, error);
  ASSERT_TRUE(!error) << error.message() << " (" << error.value() << ')';
  EXPECT_FALSE(cert.is_null());

  EXPECT_NO_THROW(cert_t::from_der(data));
}


TEST_F(crypto_certificate, from_der_empty_data)
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


TEST_F(crypto_certificate, from_der_insufficient_data)
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


TEST_F(crypto_certificate, from_der_invalid_data)
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


TEST_F(crypto_certificate, from_pem)
{
  auto data = root_cert;

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_TRUE(!error) << error.message() << " (" << error.value() << ')';
  EXPECT_FALSE(cert.is_null());

  EXPECT_NO_THROW(cert_t::from_pem(data));
}


TEST_F(crypto_certificate, from_pem_empty_data)
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


TEST_F(crypto_certificate, from_pem_insufficient_data)
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


TEST_F(crypto_certificate, from_pem_invalid_data)
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


TEST_F(crypto_certificate, from_pem_with_armor)
{
  auto data = to_armored_pem(root_cert);

  std::error_code error;
  auto cert = cert_t::from_pem(data, error);
  ASSERT_TRUE(!error) << error.message() << " (" << error.value() << ')';
  EXPECT_FALSE(cert.is_null());

  EXPECT_NO_THROW(cert_t::from_pem(data));
}


TEST_F(crypto_certificate, from_pem_with_armor_empty_data)
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


TEST_F(crypto_certificate, from_pem_with_armor_insufficient_data)
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


TEST_F(crypto_certificate, from_pem_with_armor_invalid_data)
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


TEST_F(crypto_certificate, from_pem_with_armor_invalid_data_length)
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


TEST_F(crypto_certificate, from_pem_with_armor_invalid_prefix)
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


TEST_F(crypto_certificate, from_pem_with_armor_invalid_suffix)
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


TEST_F(crypto_certificate, from_pem_with_armor_missing_suffix)
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


TEST_F(crypto_certificate, from_pem_with_too_big_data)
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


// see scripts/make_ca.sh
// notBefore=Jul 17 15:41:xx 2017 GMT
// notAfter=Jun 12 15:41:xx 2037 GMT

const std::string crypto_certificate::root_cert = // {{{1
  "MIIFjjCCA3agAwIBAgIJAJM6yoZ2pkzWMA0GCSqGSIb3DQEBCwUAMFQxCzAJBgNV"
  "BAYTAkVFMRAwDgYDVQQIDAdFc3RvbmlhMQwwCgYDVQQKDANTQUwxDzANBgNVBAsM"
  "BlNBTCBDQTEUMBIGA1UEAwwLU0FMIFJvb3QgQ0EwHhcNMTcwNzE3MTU0MTI3WhcN"
  "MzcwNzEyMTU0MTI3WjBUMQswCQYDVQQGEwJFRTEQMA4GA1UECAwHRXN0b25pYTEM"
  "MAoGA1UECgwDU0FMMQ8wDQYDVQQLDAZTQUwgQ0ExFDASBgNVBAMMC1NBTCBSb290"
  "IENBMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEApHprC/N7uFdZkQrU"
  "Uksm0+Vse2W8jGtCZWnotqpeMB7zYZGcVbF6N0gob6aNv3zjc8mPuUQFKuiLRiIg"
  "i2LADUSufhE1T3X7YxDPiGMwAGo671ytZlIVhtlXby8wgkcnl1Kh/fW9CYWgkFnx"
  "Xc9MAe90/kQAYlNT0MwAjpZ1TvJdgzJnMVaWAIDxmCTibtUpaoW/WJuDnMRw43oK"
  "ynUky2yrSN+26YbhcBD1kMJa7Mxc5LYOcqX/UU56EXw1UIfLrlxY8MJ9OY6IhQ2b"
  "DabVew2U+A5avMPwvkPit/LTzoj8mvXcsvej/TDQlFgpJ2TPdJcXmlV17rasRAxH"
  "atbm4m7rOb5UAI8cNOY/MnLfRMJWPnyMTDiRG087DnbJf+ie0uWdlSmcjUnNdBOy"
  "eP00BTxZoSkJGwMb3I7PQU4efCX0Rwe/rDi5lCBwqXq6b15Dr7rZXRyx1r3Alzxd"
  "Xzl92OOXpIV3X8EqXB6aQlvOH/5NnJ+WX5fe57g8X0gA7fBxHRv32Li28o5Ju1uQ"
  "i7C7eYxpXVamdL7tCsySgWEOdQeWWow93cJOoUw0oGM0VuypxDj7rPMjF4vXp2u9"
  "vGIioIXA6Iy5PmOJmmkfYo17wEgRf/+l1Bm0URzjWPb91O+NSNcNnb9CGnqWQzVQ"
  "FCY5dqfm/umHhTBmtUoo4dnj2cUCAwEAAaNjMGEwHQYDVR0OBBYEFNhZX8+GnMtS"
  "KZhfVfYO5Y6qJILhMB8GA1UdIwQYMBaAFNhZX8+GnMtSKZhfVfYO5Y6qJILhMA8G"
  "A1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgGGMA0GCSqGSIb3DQEBCwUAA4IC"
  "AQBoWCZjgHgG+28cFYbMjWUKW2udAwWKQxg3PcGW99OKRDkVND9GANkYs8AnwkW9"
  "ewqFhv2eb0dabCgtERs7BEaWBXPY3Hdp0SCh7Qzxr6MLphDDMkmNFlz1+8zv03jk"
  "6UWydxXJbH4JFZXoNsHUkFYWXGXSrmXGskuXUQAupyRrpYaNtztgH8yda2vV3KHH"
  "aoXMOdsLzydDygCdwz9+/Ks+/hPsoZ0fO5jglh4a/lEr80mBHBzthCOpy6tYIW3I"
  "x/Xunu0mn5nJoEMw3d5QMHijnRAbgjnQRvHnMqKKNhuItlJmxVbo6tOB2pPXbVO/"
  "/Whv70Oh2iLfey+yESlbFSOuI+7jcXG0LZTBetePmBB0PzI9I+mcd8TUNwrGk0NH"
  "XibYnvk830WmJUGWoUc/Y7vaIua8NYC7It0fD9vfLYjt8lf1DQvWfZrLHPE5iZGV"
  "pG/QVYpWRlv0/UaV+yIZzMLByp7MUGEIAzjbHAl47Y+3XdMZPatidMgv4cPnRnOJ"
  "daIuwdGMSfMMZvV8t6LaMndRZiPgKT7/gJe1Ap5L1ZCGEcLU5metgDDKYbnHFScZ"
  "WuZunCPvCNyFT/1RBiiueXvx9ErfHXQ6C+diAtbIP84vXUBVbMRTWtg09N/bjsEN"
  "7dcEtDl2bUZY1kmKhFzZt3Powd8MXdFfEqz6zW2gr7o8Zg==";

const std::string crypto_certificate::intermediate_cert = // {{{1
  "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";

const std::string crypto_certificate::leaf_cert = // {{{1
  "MIIFEzCCAvugAwIBAgICEAEwDQYJKoZIhvcNAQELBQAwXDELMAkGA1UEBhMCRUUx"
  "EDAOBgNVBAgMB0VzdG9uaWExDDAKBgNVBAoMA1NBTDEPMA0GA1UECwwGU0FMIENB"
  "MRwwGgYDVQQDDBNTQUwgSW50ZXJtZWRpYXRlIENBMB4XDTE3MDcxNzE1NDEyOVoX"
  "DTM3MDYxMjE1NDEyOVowVjELMAkGA1UEBhMCRUUxEDAOBgNVBAgMB0VzdG9uaWEx"
  "DDAKBgNVBAoMA1NBTDERMA8GA1UECwwIU0FMIFRlc3QxFDASBgNVBAMMC3Rlc3Qu"
  "c2FsLmVlMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvloCdJJRA3YI"
  "1a+ca8Gvz4fU2atr4zp8CvuzDIs87SvlovQazYQYnST1iek9Cqk2k/Oami/5o6JW"
  "ALMDHty3nwf9FcDvLp2PPtEqcqWCeRciRL2uVgQH6NiptxI8acl3awkREWaBjPEu"
  "y17jlzkmAWX2fEfnLcb4KoHAEcHABOHWbL6C4Z8fHm4bHtZJAXXpCNuCIaeR/wCd"
  "TbgmvVVpJusm3xL/uaAlDD1psLGIXCvGwp3WJM+CKfUCxoteuymjRKQLqo7dRanX"
  "zoFzwY6IzNMVXWWdygGhADXzQRVydCywAhATETkWlb2EbBU+aAQL9742pmVt9n85"
  "QFqNfMrWWQIDAQABo4HkMIHhMAkGA1UdEwQCMAAwEQYJYIZIAYb4QgEBBAQDAgZA"
  "MB0GA1UdDgQWBBQRrOQC9nQNpAoOnamWUT1VbP9z3DB9BgNVHSMEdjB0gBSaNMJV"
  "ebzavBJUQza1jR57Fr/SY6FYpFYwVDELMAkGA1UEBhMCRUUxEDAOBgNVBAgMB0Vz"
  "dG9uaWExDDAKBgNVBAoMA1NBTDEPMA0GA1UECwwGU0FMIENBMRQwEgYDVQQDDAtT"
  "QUwgUm9vdCBDQYICEAAwDgYDVR0PAQH/BAQDAgWgMBMGA1UdJQQMMAoGCCsGAQUF"
  "BwMBMA0GCSqGSIb3DQEBCwUAA4ICAQBbSL3OXRosrKVd+awT+n7INBzAQLM4GpIH"
  "8x+uomt54XOTqYYo0waDplTY+HRQWEfTM/xrk1PplbLRAwvswaG0Dth4Rm8n9Bod"
  "uqAKj4dms2IpauAwehPfBobVhdNGrtaIAv+RhtGYcZ6lrFZAxO5pIMOEYrvlPbqy"
  "yC1hPgCP7e/W4Ww0psbN4DBs3+fKZ7dpO1KasrOUK7MBoNhiubEsTuaHX0+sDkF0"
  "PMWlBi7Cq/wZ/+U77hOWOreL91Re9QQvCEmllLe7eoI2SUXXWBSwqruKqgzQZ/f/"
  "4o7oi8jdVB0o7O5mQoXwizPne3qTJLfZZvuwoltn31pcnbIByLnuXKo2iTfbkWTK"
  "SkOLBjnhsdSSeCWQLSp9vUEmtf2Br0rJ+R8hTI+AfSldj7+9nbVOP8uPV71OuBdY"
  "8lEGF9o965InE+yDHgxO2RmrNaZ5oqvHKInIW8/z9OjGSnvBLReyF/LabxwRnw1L"
  "DjO+Gwvch5K7L3k/3D/q43yHjMzZcbIBbsS4QtHCuT/lLL63MLzKlfTOsNBLn75L"
  "JCvERrkOfZ8JkVoZBk5zmDcmFqRwm8rpA4lzh0uXod2VVV9X6BrPQ/JVCjjaAHsN"
  "iR1h504PD/CwqwMb+Tu2MWBFiQMoNgLSVVySmMIESrtJRQjsMBaDRfHv33+lCWb0"
  "ZPosECETQA==";

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
