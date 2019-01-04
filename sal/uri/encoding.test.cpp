#include <sal/uri/encoding.hpp>
#include <sal/common.test.hpp>


namespace {


using uri = sal_test::fixture;


std::string decode (const std::string &in)
{
  std::string result;
  sal::uri::decode(in.begin(), in.end(), std::back_inserter(result));
  return result;
}


TEST_F(uri, decode_none)
{
  EXPECT_EQ(case_name, decode(case_name));
}


TEST_F(uri, decode_partial)
{
  EXPECT_EQ("before_\x74\x65\x73\x74_after", decode("before_%74%65%73%74_after"));
}


TEST_F(uri, decode_all)
{
  EXPECT_EQ("\x74\x65\x73\x74", decode("%74%65%73%74"));
}


TEST_F(uri, decode_empty)
{
  EXPECT_EQ("", decode(""));
}


std::error_code decode_failure (const std::string &in)
{
  try
  {
    decode(in);
    return {};
  }
  catch (const std::system_error &ex)
  {
    return ex.code();
  }
}


TEST_F(uri, decode_invalid_input)
{
  EXPECT_EQ(sal::uri::errc::invalid_hex_input, decode_failure("%0x"));
  EXPECT_EQ(sal::uri::errc::invalid_hex_input, decode_failure("%x0"));
  EXPECT_EQ(sal::uri::errc::invalid_hex_input, decode_failure("%xx"));
  EXPECT_EQ(sal::uri::errc::invalid_hex_input, decode_failure("test%xx"));
}


TEST_F(uri, decode_not_enough_data)
{
  EXPECT_EQ(sal::uri::errc::not_enough_input, decode_failure("%a"));
  EXPECT_EQ(sal::uri::errc::not_enough_input, decode_failure("a%a"));
  EXPECT_EQ(sal::uri::errc::not_enough_input, decode_failure("%ab%a"));
}


} // namespace
