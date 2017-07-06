#include <sal/encode.hpp>
#include <sal/common.test.hpp>
#include <sal/buf_ptr.hpp>


namespace {


template <typename Encoding>
using encode = sal_test::with_type<Encoding>;

using transform_types = testing::Types<
  sal::hex_string,
  sal::base64
>;

TYPED_TEST_CASE(encode, transform_types);


using encode_data = std::vector<std::pair<std::string, std::string>>;

template <typename Encoding> const encode_data expected_list = {};

template <> const encode_data expected_list<sal::hex_string> =
{
  { "hex_string", "6865785f737472696e67" },
  { "HEX_STRING", "4845585f535452494e47" },
  { "hex\nstring", "6865780a737472696e67" },
  { "hello, world", "68656c6c6f2c20776f726c64" },
};

// https://tools.ietf.org/html/rfc4648 (10)
template <> const encode_data expected_list<sal::base64> =
{
  { "", "" },
  { "f", "Zg==" },
  { "fo", "Zm8=" },
  { "foo", "Zm9v" },
  { "foob", "Zm9vYg==" },
  { "fooba", "Zm9vYmE=" },
  { "foobar", "Zm9vYmFy" },
};


TYPED_TEST(encode, max_encode_size_range)
{
  for (auto &expected: expected_list<TypeParam>)
  {
    EXPECT_EQ(expected.second.size(),
      sal::max_encoded_size<TypeParam>(
        expected.first.begin(),
        expected.first.end()
      )
    ) << "   Input: " << expected.first;
  }
}


TYPED_TEST(encode, max_encode_size_buffer)
{
  for (auto &expected: expected_list<TypeParam>)
  {
    EXPECT_EQ(expected.second.size(),
      sal::max_encoded_size<TypeParam>(expected.first)
    ) << "   Input: " << expected.first;
  }
}


TYPED_TEST(encode, encode_range_into_range)
{
  for (auto &expected: expected_list<TypeParam>)
  {
    char buf[4096];
    std::memset(buf, 0, sizeof(buf));

    auto result = std::string(buf,
      sal::encode<TypeParam>(
        expected.first.begin(),
        expected.first.end(),
        buf
      )
    );

    EXPECT_EQ(expected.second, result)
      << "   Input: " << expected.first;
  }
}


TYPED_TEST(encode, encode_buffer_into_range)
{
  for (auto &expected: expected_list<TypeParam>)
  {
    char buf[4096];
    std::memset(buf, 0, sizeof(buf));

    auto result = std::string(buf,
      sal::encode<TypeParam>(expected.first, buf)
    );

    EXPECT_EQ(expected.second, result)
      << "   Input: " << expected.first;
  }
}


TYPED_TEST(encode, encode_range_into_string)
{
  for (auto &expected: expected_list<TypeParam>)
  {
    EXPECT_EQ(expected.second,
      sal::encode<TypeParam>(expected.first.begin(), expected.first.end())
    ) << "   Input: " << expected.first;
  }
}


TYPED_TEST(encode, encode_buffer_into_string)
{
  for (auto &expected: expected_list<TypeParam>)
  {
    EXPECT_EQ(expected.second, sal::encode<TypeParam>(expected.first))
      << "   Input: " << expected.first;
  }
}


} // namespace
