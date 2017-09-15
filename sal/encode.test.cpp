#include <sal/encode.hpp>
#include <sal/common.test.hpp>
#include <sal/buf_ptr.hpp>
#include <tuple>


namespace {


template <typename Encoding>
using encode = sal_test::with_type<Encoding>;

using transform_types = testing::Types<
  sal::hex_string,
  sal::base64
>;

TYPED_TEST_CASE(encode, transform_types);


using encode_data = std::vector<std::pair<std::string, std::string>>;

template <typename Encoding> const encode_data test_data{};
template <typename Encoding> const std::string invalid_data{};
template <typename Encoding> const std::string invalid_length_data{};

template <> const encode_data test_data<sal::hex_string> =
{
  { "", "" },
  { "hex_string", "6865785f737472696e67" },
  { "HEX_STRING", "4845585f535452494e47" },
  { "hex\nstring", "6865780a737472696e67" },
  { "hello, world", "68656c6c6f2c20776f726c64" },
};

template <> const std::string invalid_data<sal::hex_string> = "xy";
template <> const std::string invalid_length_data<sal::hex_string> = "A";


// https://tools.ietf.org/html/rfc4648 (10)
template <> const encode_data test_data<sal::base64> =
{
  { "", "" },
  { "f", "Zg==" },
  { "fo", "Zm8=" },
  { "foo", "Zm9v" },
  { "foob", "Zm9vYg==" },
  { "fooba", "Zm9vYmE=" },
  { "foobar", "Zm9vYmFy" },
};

template <> const std::string invalid_data<sal::base64> = "====";
template <> const std::string invalid_length_data<sal::base64> = "A";


TYPED_TEST(encode, max_encoded_size_range)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    EXPECT_LE(
      encoded.size(),
      sal::max_encoded_size<TypeParam>(decoded.begin(), decoded.end())
    ) << "Data: " << decoded;
  }
}


TYPED_TEST(encode, max_encoded_size_buffer)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    EXPECT_LE(
      encoded.size(),
      sal::max_encoded_size<TypeParam>(decoded)
    ) << "Data: " << decoded;
  }
}


TYPED_TEST(encode, encode_range_into_range)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    char buf[4096];
    auto result = std::string(buf,
      sal::encode<TypeParam>(decoded.begin(), decoded.end(), buf)
    );

    EXPECT_EQ(encoded, result) << "Data: " << decoded;
  }
}


TYPED_TEST(encode, encode_buffer_into_range)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    char buf[4096];
    auto result = std::string(buf, sal::encode<TypeParam>(decoded, buf));

    EXPECT_EQ(encoded, result) << "Data: " << decoded;
  }
}


TYPED_TEST(encode, encode_range_into_string)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    EXPECT_EQ(encoded, sal::encode<TypeParam>(decoded.begin(), decoded.end()))
      << "Data: " << decoded;
  }
}


TYPED_TEST(encode, encode_buffer_into_string)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    EXPECT_EQ(encoded, sal::encode<TypeParam>(decoded)) << "Data: " << decoded;
  }
}


TYPED_TEST(encode, max_decoded_size_range)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    std::error_code error;
    auto size = sal::max_decoded_size<TypeParam>(
      encoded.begin(),
      encoded.end(),
      error
    );
    EXPECT_LE(decoded.size(), size) << "Data: " << decoded;
    EXPECT_TRUE(!error);

    EXPECT_NO_THROW(
      sal::max_decoded_size<TypeParam>(encoded.begin(), encoded.end())
    );
  }
}


TYPED_TEST(encode, max_decoded_size_buffer)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    std::error_code error;
    auto size = sal::max_decoded_size<TypeParam>(encoded, error);
    EXPECT_LE(decoded.size(), size) << "Data: " << decoded;
    EXPECT_TRUE(!error);

    EXPECT_NO_THROW(sal::max_decoded_size<TypeParam>(encoded));
  }
}


TYPED_TEST(encode, max_decoded_size_range_invalid)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded += invalid_length_data<TypeParam>;

    std::error_code error;
    sal::max_decoded_size<TypeParam>(encoded.begin(), encoded.end(), error);
    EXPECT_EQ(std::errc::message_size, error) << "Data: " << decoded;

    EXPECT_THROW(
      sal::max_decoded_size<TypeParam>(encoded.begin(), encoded.end()),
      std::system_error
    ) << "Data: " << decoded;
  }
}


TYPED_TEST(encode, max_decoded_size_buffer_invalid)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded += invalid_length_data<TypeParam>;

    std::error_code error;
    sal::max_decoded_size<TypeParam>(encoded, error);
    EXPECT_EQ(std::errc::message_size, error) << "Data: " << decoded;

    EXPECT_THROW(sal::max_decoded_size<TypeParam>(encoded), std::system_error)
      << "Data: " << decoded;
  }
}


TYPED_TEST(encode, decode_range_into_range)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    char buf[4096];
    std::error_code error;
    auto result = std::string(buf,
      sal::decode<TypeParam>(encoded.begin(), encoded.end(), buf, error)
    );

    EXPECT_TRUE(!error);
    EXPECT_EQ(decoded, result) << "Data: " << decoded;

    EXPECT_NO_THROW(
      sal::decode<TypeParam>(encoded.begin(), encoded.end(), buf)
    );
  }
}


TYPED_TEST(encode, decode_range_into_range_invalid_length)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded += invalid_length_data<TypeParam>;

    char buf[4096];
    std::error_code error;
    auto result = std::string(buf,
      sal::decode<TypeParam>(encoded.begin(), encoded.end(), buf, error)
    );

    EXPECT_EQ(std::errc::message_size, error) << error.message();
    EXPECT_TRUE(result.empty());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded.begin(), encoded.end(), buf),
      std::system_error
    );
  }
}


TYPED_TEST(encode, decode_range_into_range_invalid_data_in_front)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded = invalid_data<TypeParam> + encoded;

    char buf[4096];
    std::error_code error;
    auto result = std::string(buf,
      sal::decode<TypeParam>(encoded.begin(), encoded.end(), buf, error)
    );

    EXPECT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
    EXPECT_TRUE(result.empty());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded.begin(), encoded.end(), buf),
      std::system_error
    );
  }
}


TYPED_TEST(encode, decode_range_into_range_invalid_data_in_back)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded += invalid_data<TypeParam>;

    char buf[4096];
    std::error_code error;
    auto result = std::string(buf,
      sal::decode<TypeParam>(encoded.begin(), encoded.end(), buf, error)
    );

    EXPECT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
    EXPECT_EQ(decoded.size(), result.size());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded.begin(), encoded.end(), buf),
      std::system_error
    );
  }
}


TYPED_TEST(encode, decode_buffer_into_range)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    char buf[4096];
    std::error_code error;
    auto result = std::string(buf, sal::decode<TypeParam>(encoded, buf, error));

    EXPECT_TRUE(!error);
    EXPECT_EQ(decoded, result) << "Data: " << decoded;

    EXPECT_NO_THROW(sal::decode<TypeParam>(encoded, buf));
  }
}


TYPED_TEST(encode, decode_buffer_into_range_invalid_length)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded += invalid_length_data<TypeParam>;

    char buf[4096];
    std::error_code error;
    auto result = std::string(buf, sal::decode<TypeParam>(encoded, buf, error));

    EXPECT_EQ(std::errc::message_size, error) << error.message();
    EXPECT_TRUE(result.empty());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded, buf),
      std::system_error
    );
  }
}


TYPED_TEST(encode, decode_buffer_into_range_invalid_data_in_front)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded = invalid_data<TypeParam> + encoded;

    char buf[4096];
    std::error_code error;
    auto result = std::string(buf, sal::decode<TypeParam>(encoded, buf, error));

    EXPECT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
    EXPECT_TRUE(result.empty());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded, buf),
      std::system_error
    );
  }
}


TYPED_TEST(encode, decode_buffer_into_range_invalid_data_in_back)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded += invalid_data<TypeParam>;

    char buf[4096];
    std::error_code error;
    auto result = std::string(buf, sal::decode<TypeParam>(encoded, buf, error));

    EXPECT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
    EXPECT_EQ(decoded.size(), result.size());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded, buf),
      std::system_error
    );
  }
}


inline auto to_string (const std::vector<uint8_t> &in)
{
  return std::string(in.begin(), in.end());
}


TYPED_TEST(encode, decode_range_into_vector)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    std::error_code error;
    auto result = sal::decode<TypeParam>(encoded.begin(), encoded.end(), error);

    EXPECT_TRUE(!error);
    EXPECT_EQ(decoded, to_string(result)) << "Data: " << decoded;

    EXPECT_NO_THROW(sal::decode<TypeParam>(encoded.begin(), encoded.end()));
  }
}


TYPED_TEST(encode, decode_range_into_vector_invalid_length)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded += invalid_length_data<TypeParam>;

    std::error_code error;
    auto result = sal::decode<TypeParam>(encoded.begin(), encoded.end(), error);

    EXPECT_EQ(std::errc::message_size, error) << error.message();
    EXPECT_TRUE(result.empty());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded.begin(), encoded.end()),
      std::system_error
    );
  }
}


TYPED_TEST(encode, decode_range_into_vector_invalid_data_in_front)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded = invalid_data<TypeParam> + encoded;

    std::error_code error;
    auto result = sal::decode<TypeParam>(encoded.begin(), encoded.end(), error);

    EXPECT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
    EXPECT_TRUE(result.empty());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded.begin(), encoded.end()),
      std::system_error
    );
  }
}


TYPED_TEST(encode, decode_range_into_vector_invalid_data_in_back)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded += invalid_data<TypeParam>;

    std::error_code error;
    auto result = sal::decode<TypeParam>(encoded.begin(), encoded.end(), error);

    EXPECT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
    EXPECT_EQ(decoded.size(), result.size());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded.begin(), encoded.end()),
      std::system_error
    );
  }
}


TYPED_TEST(encode, decode_buffer_into_vector)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;

    std::error_code error;
    auto result = sal::decode<TypeParam>(encoded, error);

    EXPECT_TRUE(!error);
    EXPECT_EQ(decoded, to_string(result)) << "Data: " << decoded;

    EXPECT_NO_THROW(sal::decode<TypeParam>(encoded));
  }
}


TYPED_TEST(encode, decode_buffer_into_vector_invalid_length)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded += invalid_length_data<TypeParam>;

    std::error_code error;
    auto result = sal::decode<TypeParam>(encoded, error);

    EXPECT_EQ(std::errc::message_size, error) << error.message();
    EXPECT_TRUE(result.empty());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded),
      std::system_error
    );
  }
}


TYPED_TEST(encode, decode_buffer_into_vector_invalid_data_in_front)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded = invalid_data<TypeParam> + encoded;

    std::error_code error;
    auto result = sal::decode<TypeParam>(encoded, error);

    EXPECT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
    EXPECT_TRUE(result.empty());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded),
      std::system_error
    );
  }
}


TYPED_TEST(encode, decode_buffer_into_vector_invalid_data_in_back)
{
  std::string decoded, encoded;
  for (auto &data: test_data<TypeParam>)
  {
    std::tie(decoded, encoded) = data;
    encoded += invalid_data<TypeParam>;

    std::error_code error;
    auto result = sal::decode<TypeParam>(encoded, error);

    EXPECT_EQ(std::errc::illegal_byte_sequence, error) << error.message();
    EXPECT_EQ(decoded.size(), result.size());

    EXPECT_THROW(
      sal::decode<TypeParam>(encoded),
      std::system_error
    );
  }
}


} // namespace
