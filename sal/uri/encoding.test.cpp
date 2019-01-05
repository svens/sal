#include <sal/uri/encoding.hpp>
#include <sal/common.test.hpp>
#include <map>
#include <unordered_map>


namespace {


using uri = sal_test::fixture;
using namespace std::string_view_literals;


// decode {{{1


TEST_F(uri, decode_none)
{
  EXPECT_EQ(case_name, sal::uri::decode(case_name));
}


TEST_F(uri, decode_partial)
{
  EXPECT_EQ("before_\x74\x65\x73\x74_after",
    sal::uri::decode("before_%74%65%73%74_after"sv)
  );
}


TEST_F(uri, decode_all)
{
  EXPECT_EQ("\x74\x65\x73\x74", sal::uri::decode("%74%65%73%74"sv));
  EXPECT_EQ("\xaf\xaf\xaf\xaf", sal::uri::decode("%af%Af%aF%AF"sv));
}


TEST_F(uri, decode_empty)
{
  EXPECT_EQ("", sal::uri::decode(""sv));
}


std::error_code decode_failure (std::string_view data)
{
  try
  {
    sal::uri::decode(data);
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
    sal::uri::encode_user_info("u-s.e_r:i~n1f9%20o"sv)
  );
}


TEST_F(uri, encode_user_info_partial)
{
  EXPECT_EQ("%7B%80user%AAinfo%FF%7D",
    sal::uri::encode_user_info("{\x80user\xaainfo\xff}"sv)
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
  EXPECT_EQ("", sal::uri::encode_user_info(""sv));
}


// encode_path {{{1


TEST_F(uri, encode_path_none)
{
  EXPECT_EQ("/test/../%20:%20@path;p=v",
    sal::uri::encode_path("/test/../ :%20@path;p=v"sv)
  );
}


TEST_F(uri, encode_path_partial)
{
  EXPECT_EQ("/%80test/../%20:%AApath@%FF%7B;p=v%7D",
    sal::uri::encode_path("/\x80test/../ :\xaapath@\xff{;p=v}"sv)
  );
}


TEST_F(uri, encode_path_all)
{
  std::ostringstream expected;
  expected << std::uppercase;
  std::string input;
  for (uint16_t ch = 0x80;  ch < 0x100;  ++ch)
  {
    input.push_back(static_cast<uint8_t>(ch));
    expected << '%' << std::hex << ch;
  }
  EXPECT_EQ(expected.str(), sal::uri::encode_path(input));
}


TEST_F(uri, encode_path_empty)
{
  EXPECT_EQ("", sal::uri::encode_path(""sv));
}


// encode_query {{{1


TEST_F(uri, encode_query_none)
{
  EXPECT_EQ("?k1=v1&k2=v2/k3=v3",
    sal::uri::encode_query("?k1=v1&k2=v2/k3=v3"sv)
  );
}


TEST_F(uri, encode_query_partial)
{
  EXPECT_EQ("?%81k1=v1&%AAk2=v2%FF%20/%7Bk3=v3%7D",
    sal::uri::encode_query("?\x81k1=v1&\xaak2=v2\xff /{k3=v3}"sv)
  );
}


TEST_F(uri, encode_query_all)
{
  std::ostringstream expected;
  expected << std::uppercase;
  std::string input;
  for (uint16_t ch = 0x80;  ch < 0x100;  ++ch)
  {
    input.push_back(static_cast<uint8_t>(ch));
    expected << '%' << std::hex << ch;
  }
  EXPECT_EQ(expected.str(), sal::uri::encode_query(input));
}


TEST_F(uri, encode_query_empty)
{
  EXPECT_EQ("", sal::uri::encode_query(""sv));
}


TEST_F(uri, encode_query_map)
{
  std::map<std::string, std::string> map =
  {
    { "one", "1" },
    { "two", "2" },
    { "t=t", "3" },
  };
  auto result = sal::uri::encode_query(map);
  EXPECT_NE(std::string::npos, result.find("one=1"));
  EXPECT_NE(std::string::npos, result.find("two=2"));
  EXPECT_NE(std::string::npos, result.find("t%3Dt=3"));
}


TEST_F(uri, encode_query_empty_map)
{
  std::map<std::string, std::string> map =
  { };
  EXPECT_EQ("", sal::uri::encode_query(map));
}


TEST_F(uri, encode_query_unordered_map)
{
  std::unordered_map<std::string, std::string> map =
  {
    { "one", "1" },
    { "two", "2" },
    { "t=t", "3" },
  };
  auto result = sal::uri::encode_query(map);
  EXPECT_NE(std::string::npos, result.find("one=1"));
  EXPECT_NE(std::string::npos, result.find("two=2"));
  EXPECT_NE(std::string::npos, result.find("t%3Dt=3"));
}


TEST_F(uri, encode_query_empty_unordered_map)
{
  std::unordered_map<std::string, std::string> map =
  { };
  EXPECT_EQ("", sal::uri::encode_query(map));
}


// encode_fragment {{{1


TEST_F(uri, encode_fragment_none)
{
  EXPECT_EQ("/f%20/%20ragment@?",
    sal::uri::encode_fragment("/f%20/ ragment@?"sv)
  );
}


TEST_F(uri, encode_fragment_partial)
{
  EXPECT_EQ("%81/f%20%AA/%20ragment%FF@?",
    sal::uri::encode_fragment("\x81/f%20\xaa/ ragment\xff@?"sv)
  );
}


TEST_F(uri, encode_fragment_all)
{
  std::ostringstream expected;
  expected << std::uppercase;
  std::string input;
  for (uint16_t ch = 0x80;  ch < 0x100;  ++ch)
  {
    input.push_back(static_cast<uint8_t>(ch));
    expected << '%' << std::hex << ch;
  }
  EXPECT_EQ(expected.str(), sal::uri::encode_fragment(input));
}


TEST_F(uri, encode_fragment_empty)
{
  EXPECT_EQ("", sal::uri::encode_fragment(""sv));
}


//}}}1


} // namespace
