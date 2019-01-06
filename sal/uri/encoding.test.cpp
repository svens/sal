#include <sal/uri/encoding.hpp>
#include <sal/common.test.hpp>
#include <functional>
#include <map>
#include <unordered_map>


namespace {


using namespace std::string_literals;


// decode_success {{{1


struct decode_success_t
{
  std::string data, expected;

  friend std::ostream &operator<< (std::ostream &os, const decode_success_t &test)
  {
    return (os << '\'' << test.data << '\'');
  }
};


decode_success_t decode_success_data[] =
{
  { "", "" },
  { "a_%74%65%73%74_z", "a_test_z" },
  { "%74%65%73%74_data_%74%65%73%74", "test_data_test" },
  { "%74%65%73%74%af%Af%aF%AF", "\x74\x65\x73\x74\xaf\xaf\xaf\xaf" },
};


using encoding_decode_success = ::testing::TestWithParam<decode_success_t>;
INSTANTIATE_TEST_CASE_P(uri,
  encoding_decode_success,
  ::testing::ValuesIn(decode_success_data),
);


TEST_P(encoding_decode_success, test)
{
  auto &test = GetParam();
  EXPECT_EQ(test.expected, sal::uri::decode(test.data));
}


// decode_fail {{{1


struct decode_fail_t
{
  std::string data;
  sal::uri::errc expected;

  friend std::ostream &operator<< (std::ostream &os, const decode_fail_t &test)
  {
    return (os << '\'' << test.data << '\'');
  }
};


decode_fail_t decode_fail_data[] =
{
  { "%0x", sal::uri::errc::invalid_hex_input },
  { "%x0", sal::uri::errc::invalid_hex_input },
  { "%xx", sal::uri::errc::invalid_hex_input },
  { "test%xx", sal::uri::errc::invalid_hex_input },
  { "%a", sal::uri::errc::not_enough_input },
  { "a%a", sal::uri::errc::not_enough_input },
  { "%ab%a", sal::uri::errc::not_enough_input },
};


using encoding_decode_fail = ::testing::TestWithParam<decode_fail_t>;
INSTANTIATE_TEST_CASE_P(uri,
  encoding_decode_fail,
  ::testing::ValuesIn(decode_fail_data),
);


TEST_P(encoding_decode_fail, test)
{
  auto &test = GetParam();
  auto expected_error = sal::uri::make_error_code(test.expected);
  try
  {
    sal::uri::decode(test.data);
    FAIL() << "expected decoding failure (" << expected_error << ")";
  }
  catch (const std::system_error &ex)
  {
    EXPECT_EQ(expected_error, ex.code());
  }
}


// encode {{{1


struct encode_success_t
{
  std::function<std::string(std::string)> encoder;
  std::string data, expected;

  friend std::ostream &operator<< (std::ostream &os, const encode_success_t &test)
  {
    return (os << '\'' << test.data << '\'');
  }
};


encode_success_t encode_success_data[] =
{
  // user_info
  {
    sal::uri::encode_user_info<std::string>,
    "",
    ""
  },
  {
    sal::uri::encode_user_info<std::string>,
    "u-._:~%20o",
    "u-._:~%20o"
  },
  {
    sal::uri::encode_user_info<std::string>,
    "{\x00\x01}"s,
    "%7B%00%01%7D"
  },

  // path
  {
    sal::uri::encode_path<std::string>,
    "",
    ""
  },
  {
    sal::uri::encode_path<std::string>,
    "/test/../:%20@path;p=v",
    "/test/../:%20@path;p=v"
  },
  { sal::uri::encode_path<std::string>,
    "/{\x00\x01}\\/\xff"s,
    "/%7B%00%01%7D%5C/%FF"s,
  },

  // query
  {
    sal::uri::encode_query<std::string>,
    "",
    ""
  },
  {
    sal::uri::encode_query<std::string>,
    "?test=&k1=v1/:%20;p=v",
    "?test=&k1=v1/:%20;p=v"
  },
  { sal::uri::encode_query<std::string>,
    "{\x00\x01}?k1=v1 @\xaa "s,
    "%7B%00%01%7D?k1=v1%20@%AA%20",
  },

  // fragment
  {
    sal::uri::encode_fragment<std::string>,
    "",
    ""
  },
  {
    sal::uri::encode_fragment<std::string>,
    "?test=&k1=v1?/:%20;p=v",
    "?test=&k1=v1?/:%20;p=v"
  },
  { sal::uri::encode_fragment<std::string>,
    "{\x00\x01}#?k1=v1 @\xaa "s,
    "%7B%00%01%7D%23?k1=v1%20@%AA%20",
  },
};


using encoding_encode_success = ::testing::TestWithParam<encode_success_t>;
INSTANTIATE_TEST_CASE_P(uri,
  encoding_encode_success,
  ::testing::ValuesIn(encode_success_data),
);


TEST_P(encoding_encode_success, test)
{
  auto &test = GetParam();
  EXPECT_EQ(test.expected, test.encoder(test.data));
}


// encode_map {{{1


using uri = sal_test::fixture;


template <template <typename, typename> typename Map>
Map<std::string, std::string> test_map ()
{
  return
  {
    { "none", "n" },
    { "\x80pa=rt&ial\xff", "p" },
    { "\x80\xff", "a" },
    { "", "vempty" },
    { "kempty", "" },
  };
}


void check_expected_result (const std::string &result)
{
  EXPECT_NE(std::string::npos, result.find("none=n"));
  EXPECT_NE(std::string::npos, result.find("%80pa%3Drt%26ial%FF=p"));
  EXPECT_NE(std::string::npos, result.find("%80%FF=a"));
  EXPECT_NE(std::string::npos, result.find("=vempty"));
  EXPECT_NE(std::string::npos, result.find("kempty="));
}


TEST_F(uri, encoding_encode_query_map)
{
  auto result = sal::uri::encode_query(test_map<std::map>());
  check_expected_result(result);
}


TEST_F(uri, encoding_encode_query_map_empty)
{
  std::map<std::string, std::string> map =
  { };
  EXPECT_EQ("", sal::uri::encode_query(map));
}


TEST_F(uri, encoding_encode_query_unordered_map)
{
  auto result = sal::uri::encode_query(test_map<std::unordered_map>());
  check_expected_result(result);
}


TEST_F(uri, encoding_encode_query_unordered_map_empty)
{
  std::unordered_map<std::string, std::string> map =
  { };
  EXPECT_EQ("", sal::uri::encode_query(map));
}


//}}}1


} // namespace
