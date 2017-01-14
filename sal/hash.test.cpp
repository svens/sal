#include <sal/hash.hpp>
#include <sal/common.test.hpp>


namespace {


TEST(hash, fnv_1a_64)
{
  char data[] = "0123";
  auto a = sal::fnv_1a_64(data, data + 4);

  data[3]++;
  auto b = sal::fnv_1a_64(data, data + 4);

  // expect differ more than last bit position
  EXPECT_NE(1U, a^b);
}


TEST(hash, hash_128_to_64)
{
  char data[] = "0123";
  auto a = sal::fnv_1a_64(data, data + 4);

  auto b = sal::hash_128_to_64(1,
    sal::fnv_1a_64(data, data + 4)
  );

  // expect differ more than last bit position
  EXPECT_NE(1U, a^b);
}


} // namespace
