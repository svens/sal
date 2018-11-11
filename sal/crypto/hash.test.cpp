#include <sal/crypto/hash.hpp>
#include <sal/crypto/common.test.hpp>
#include <map>
#include <string>
#include <vector>


namespace {


template <typename Algorithm>
using crypto_hash = sal_test::with_type<Algorithm>;

TYPED_TEST_CASE(crypto_hash, sal_test::digest_types, sal_test::digest_names);


std::string empty = "",
  lazy_dog = "The quick brown fox jumps over the lazy dog",
  lazy_cog = "The quick brown fox jumps over the lazy cog";


using string_map = std::map<std::string, std::string>;

template <typename Algorithm>
const bool expected = false;

string_map md5 =
{
  { empty, "d41d8cd98f00b204e9800998ecf8427e" },
  { lazy_dog, "9e107d9d372bb6826bd81d3542a419d6" },
  { lazy_cog, "1055d3e698d289f2af8663725127bd4b" },
  { lazy_dog + lazy_cog, "29b4e7d924350ff800471c80c9ca2a3f" },
};
template <> string_map &expected<sal::crypto::md5> = md5;

string_map sha1 =
{
  { empty, "da39a3ee5e6b4b0d3255bfef95601890afd80709" },
  { lazy_dog, "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12" },
  { lazy_cog, "de9f2c7fd25e1b3afad3e85a0bd17d9b100db4b3" },
  { lazy_dog + lazy_cog, "38590c861cc71a4186b2909285a04609fb23bb42" },
};
template <> string_map &expected<sal::crypto::sha1> = sha1;

string_map sha256 =
{
  { empty, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" },
  { lazy_dog, "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592" },
  { lazy_cog, "e4c4d8f3bf76b692de791a173e05321150f7a345b46484fe427f6acc7ecc81be" },
  { lazy_dog + lazy_cog, "0a9a361e469fd8fb48e915a06431f3fabbfb0960226421a25ab939fde121b7c8" },
};
template <> string_map &expected<sal::crypto::sha256> = sha256;

string_map sha384 =
{
  { empty, "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b" },
  { lazy_dog, "ca737f1014a48f4c0b6dd43cb177b0afd9e5169367544c494011e3317dbf9a509cb1e5dc1e85a941bbee3d7f2afbc9b1" },
  { lazy_cog, "098cea620b0978caa5f0befba6ddcf22764bea977e1c70b3483edfdf1de25f4b40d6cea3cadf00f809d422feb1f0161b" },
  { lazy_dog + lazy_cog, "03b251e870443c1dc8052967970cc91bdd3bd5c3784ea0b2df52f0f4a6c56f947fcc1369b593730479dc07d73a043297" },
};
template <> string_map &expected<sal::crypto::sha384> = sha384;

string_map sha512 =
{
  { empty, "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e" },
  { lazy_dog, "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb642e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6" },
  { lazy_cog, "3eeee1d0e11733ef152a6c29503b3ae20c4f1f3cda4cb26f1bc1a41f91c7fe4ab3bd86494049e201c4bd5155f31ecb7a3c8606843c4cc8dfcab7da11c8ae5045" },
  { lazy_dog + lazy_cog, "9a1eacc4b2de80d412e8e28aa918c22450246c9d249559e6cba45145feebd05298c8d91cde493acd7c2bf9ed5c86612a7f8c8323c10913d8b4703c8d6bcd99f8" },
};
template <> string_map &expected<sal::crypto::sha512> = sha512;


template <typename Ptr>
std::string to_string (const Ptr &data)
{
  std::string result;
  for (auto &b: data)
  {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02x", static_cast<uint32_t>(b));
    result += buf;
  }
  return result;
}


TYPED_TEST(crypto_hash, copy_ctor)
{
  sal::crypto::hash_t<TypeParam> h1;
  h1.update(lazy_dog);
  auto h2 = h1;

  h1.update(lazy_cog);
  h2.update(lazy_cog);

  EXPECT_EQ(expected<TypeParam>[lazy_dog + lazy_cog], to_string(h1.finish()));
  EXPECT_EQ(expected<TypeParam>[lazy_dog + lazy_cog], to_string(h2.finish()));
}


TYPED_TEST(crypto_hash, copy_assign)
{
  sal::crypto::hash_t<TypeParam> h1, h2;
  h1.update(lazy_dog);
  h2 = h1;

  h1.update(lazy_cog);
  h2.update(lazy_cog);

  EXPECT_EQ(expected<TypeParam>[lazy_dog + lazy_cog], to_string(h1.finish()));
  EXPECT_EQ(expected<TypeParam>[lazy_dog + lazy_cog], to_string(h2.finish()));
}


TYPED_TEST(crypto_hash, move_ctor)
{
  sal::crypto::hash_t<TypeParam> h1;
  h1.update(lazy_dog);

  auto h2{std::move(h1)};
  h2.update(lazy_cog);

  EXPECT_EQ(expected<TypeParam>[lazy_dog + lazy_cog], to_string(h2.finish()));
}


TYPED_TEST(crypto_hash, move_assign)
{
  sal::crypto::hash_t<TypeParam> h1, h2;
  h1.update(lazy_dog);

  h2 = std::move(h1);
  h2.update(lazy_cog);

  EXPECT_EQ(expected<TypeParam>[lazy_dog + lazy_cog], to_string(h2.finish()));
}


TYPED_TEST(crypto_hash, no_add)
{
  sal::crypto::hash_t<TypeParam> hash;
  EXPECT_NE(0U, hash.digest_size);
  EXPECT_EQ(expected<TypeParam>[empty], to_string(hash.finish()));
}


TYPED_TEST(crypto_hash, invalid_result_size)
{
  sal::crypto::hash_t<TypeParam> hash;
  uint8_t result[sal::crypto::hash_t<TypeParam>::digest_size / 2];
  EXPECT_THROW(
    hash.finish(std::begin(result), std::end(result)),
    std::logic_error
  );
}


TYPED_TEST(crypto_hash, reuse_object)
{
  sal::crypto::hash_t<TypeParam> hash;

  hash.update(empty);
  EXPECT_EQ(expected<TypeParam>[empty], to_string(hash.finish()));

  hash.update(lazy_dog);
  EXPECT_EQ(expected<TypeParam>[lazy_dog], to_string(hash.finish()));

  hash.update(lazy_cog);
  EXPECT_EQ(expected<TypeParam>[lazy_cog], to_string(hash.finish()));
}


TYPED_TEST(crypto_hash, multiple_updates)
{
  sal::crypto::hash_t<TypeParam> hash;

  hash.update(lazy_dog).update(lazy_cog);
  EXPECT_EQ(expected<TypeParam>[lazy_dog + lazy_cog], to_string(hash.finish()));

  hash.update(lazy_dog + lazy_cog);
  EXPECT_EQ(expected<TypeParam>[lazy_dog + lazy_cog], to_string(hash.finish()));
}


TYPED_TEST(crypto_hash, multiple_instances)
{
  sal::crypto::hash_t<TypeParam> dog, cog;
  dog.update(lazy_dog);
  cog.update(lazy_cog);
  EXPECT_EQ(expected<TypeParam>[lazy_dog], to_string(dog.finish()));
  EXPECT_EQ(expected<TypeParam>[lazy_cog], to_string(cog.finish()));
}


TYPED_TEST(crypto_hash, input_range_output_range)
{
  sal::crypto::hash_t<TypeParam> hash;
  for (auto &kv: expected<TypeParam>)
  {
    hash.update(kv.first.begin(), kv.first.end());
    std::array<uint8_t, hash.digest_size> result;
    hash.finish(result.begin(), result.end());
    EXPECT_EQ(kv.second, to_string(result));
  }
}


TYPED_TEST(crypto_hash, input_range_output_array)
{
  sal::crypto::hash_t<TypeParam> hash;
  for (auto &kv: expected<TypeParam>)
  {
    hash.update(kv.first.begin(), kv.first.end());
    EXPECT_EQ(kv.second, to_string(hash.finish()));
  }
}


TYPED_TEST(crypto_hash, input_string_output_range)
{
  sal::crypto::hash_t<TypeParam> hash;
  for (auto &kv: expected<TypeParam>)
  {
    hash.update(kv.first);
    std::array<uint8_t, hash.digest_size> result;
    hash.finish(result.begin(), result.end());
    EXPECT_EQ(kv.second, to_string(result));
  }
}


TYPED_TEST(crypto_hash, input_string_output_array)
{
  sal::crypto::hash_t<TypeParam> hash;
  for (auto &kv: expected<TypeParam>)
  {
    hash.update(kv.first);
    EXPECT_EQ(kv.second, to_string(hash.finish()));
  }
}


TYPED_TEST(crypto_hash, vector)
{
  sal::crypto::hash_t<TypeParam> hash;
  for (auto &kv: expected<TypeParam>)
  {
    hash.update(std::vector<uint8_t>{kv.first.begin(), kv.first.end()});
    EXPECT_EQ(kv.second, to_string(hash.finish()));
  }
}


TYPED_TEST(crypto_hash, one_shot_input_range_output_range)
{
  for (auto &kv: expected<TypeParam>)
  {
    uint8_t result[sal::crypto::hash_t<TypeParam>::digest_size];
    sal::crypto::hash_t<TypeParam>::one_shot(
      kv.first.cbegin(), kv.first.cend(),
      std::begin(result), std::end(result)
    );
    EXPECT_EQ(kv.second, to_string(result));
  }
}


TYPED_TEST(crypto_hash, one_shot_input_range_output_array)
{
  for (auto &kv: expected<TypeParam>)
  {
    EXPECT_EQ(kv.second,
      to_string(
        sal::crypto::hash_t<TypeParam>::one_shot(
          kv.first.cbegin(),
          kv.first.cend()
        )
      )
    );
  }
}


TYPED_TEST(crypto_hash, one_shot_input_string_output_array)
{
  for (auto &kv: expected<TypeParam>)
  {
    EXPECT_EQ(kv.second,
      to_string(sal::crypto::hash_t<TypeParam>::one_shot(kv.first))
    );
  }
}


} // namespace
