#pragma once

#include <sal/common.test.hpp>
#include <sal/crypto/hash.hpp>
#include <sal/encode.hpp>
#include <string>
#include <vector>


namespace sal_test {


using digest_types = ::testing::Types<
  sal::crypto::md5,
  sal::crypto::sha1,
  sal::crypto::sha256,
  sal::crypto::sha384,
  sal::crypto::sha512
>;


struct digest_names
{
  template <typename T>
  static std::string GetName (int i)
  {
    (void)i;
    if constexpr (std::is_same_v<T, sal::crypto::md5>)
    {
      return "md5";
    }
    else if constexpr (std::is_same_v<T, sal::crypto::sha1>)
    {
      return "sha1";
    }
    else if constexpr (std::is_same_v<T, sal::crypto::sha256>)
    {
      return "sha256";
    }
    else if constexpr (std::is_same_v<T, sal::crypto::sha384>)
    {
      return "sha384";
    }
    else if constexpr (std::is_same_v<T, sal::crypto::sha512>)
    {
      return "sha512";
    }
    else
    {
      return std::to_string(i);
    }
  }
};


namespace cert {

extern const std::string
    // generated chain
    root,
    intermediate,
    leaf,

    // above as base64 encoded PKCS12 bytes
    pkcs12,
    pkcs12_no_passphrase,

    // specific testcase certificates
    without_key_id;

} // namespace cert


inline std::vector<uint8_t> to_der (const std::string &base64)
{
  return sal::decode<sal::base64>(base64);
}


inline std::string to_pem (const std::string &base64)
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


} // namespace sal_test
