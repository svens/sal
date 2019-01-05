#include <sal/uri/encoding.hpp>
#include <sal/common.test.hpp>


namespace {


using uri = sal_test::fixture;
using namespace std::string_literals;


// decode {{{1


TEST_F(uri, decode_none)
{
  EXPECT_EQ(case_name, sal::uri::decode(case_name));
}


TEST_F(uri, decode_partial)
{
  EXPECT_EQ("before_\x74\x65\x73\x74_after",
    sal::uri::decode("before_%74%65%73%74_after"s)
  );
}


TEST_F(uri, decode_all)
{
  EXPECT_EQ("\x74\x65\x73\x74", sal::uri::decode("%74%65%73%74"s));
  EXPECT_EQ("\xaf\xaf\xaf\xaf", sal::uri::decode("%af%Af%aF%AF"s));
}


TEST_F(uri, decode_empty)
{
  EXPECT_EQ("", sal::uri::decode(""s));
}


std::error_code decode_failure (const std::string &in)
{
  try
  {
    sal::uri::decode(in);
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


// encode_user_info {{{1


TEST_F(uri, encode_user_info_none)
{
  EXPECT_EQ("u-s.e_r:i~n1f9%20o",
    sal::uri::encode_user_info("u-s.e_r:i~n1f9%20o"s)
  );
}


TEST_F(uri, encode_user_info_partial)
{
  EXPECT_EQ("%80user%AAinfo%FF",
    sal::uri::encode_user_info("\x80user\xaainfo\xff"s)
  );
}


TEST_F(uri, encode_user_info_all)
{
  std::ostringstream expected;
  expected << std::uppercase;
  std::string input;
  for (uint16_t ch = 0x80;  ch < 0x100;  ++ch)
  {
    input.push_back(static_cast<uint8_t>(ch));
    expected << '%' << std::hex << ch;
  }
  EXPECT_EQ(expected.str(), sal::uri::encode_user_info(input));
}


TEST_F(uri, encode_user_info_empty)
{
  EXPECT_EQ("", sal::uri::encode_user_info(""s));
}


//}}}1


} // namespace
