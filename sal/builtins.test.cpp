#include <sal/builtins.hpp>
#include <sal/common.test.hpp>


TEST(builtins, sal_likely)
{
  EXPECT_FALSE(sal_likely(false));
  EXPECT_TRUE(sal_likely(true));
}


TEST(builtins, sal_unlikely)
{
  EXPECT_FALSE(sal_unlikely(false));
  EXPECT_TRUE(sal_unlikely(true));
}


TEST(builtins, sal_clz)
{
  for (uint64_t i = 0;  i != 64;  ++i)
  {
    EXPECT_EQ(63 - i, sal_clz(1ULL << i));
  }
}
