#pragma once

#include <sal/common.test.hpp>
#include <sal/encode.hpp>
#include <string>
#include <vector>


namespace sal_test {


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
    too_long_chain,
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
