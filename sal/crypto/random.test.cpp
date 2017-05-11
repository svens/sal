#include <sal/crypto/random.hpp>
#include <sal/common.test.hpp>
#include <sal/buf_ptr.hpp>


namespace {


using crypto_random = sal_test::fixture;


TEST_F(crypto_random, vector)
{
  std::vector<unsigned> expected{ 3, 1, 4, 1, 5, 9, 2, 6 };
  auto data = expected;
  sal::crypto::random(sal::make_buf(data));
  EXPECT_NE(expected, data);
}


TEST_F(crypto_random, empty_vector)
{
  std::vector<unsigned> data;
  EXPECT_THROW(
    sal::crypto::random(sal::make_buf(data)),
    std::logic_error
  );
  EXPECT_TRUE(data.empty());
}


TEST_F(crypto_random, string)
{
  auto data = case_name;
  sal::crypto::random(sal::make_buf(data));
  EXPECT_NE(case_name, data);
}


TEST_F(crypto_random, empty_string)
{
  std::vector<unsigned> data;
  EXPECT_THROW(
    sal::crypto::random(sal::make_buf(data)),
    std::logic_error
  );
  EXPECT_TRUE(data.empty());
}


} // namespace
