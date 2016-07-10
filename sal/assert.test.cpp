#include <sal/assert.hpp>
#include <sal/common.test.hpp>


TEST(assert, true)
{
  EXPECT_NO_THROW(sal_assert(true));
}


TEST(assert, false)
{
#if !defined(NDEBUG)
  EXPECT_THROW(sal_assert(false), std::logic_error);
#else
  EXPECT_NO_THROW(sal_assert(false));
#endif
}


TEST(verify, true)
{
  auto value = false;
  EXPECT_NO_THROW(sal_verify(value = true));
  EXPECT_TRUE(value);
}


TEST(verify, false)
{
  auto value = true;
#if !defined(NDEBUG)
  EXPECT_THROW(sal_verify(value = false), std::logic_error);
#else
  EXPECT_NO_THROW(sal_verify(value = false));
#endif
  EXPECT_FALSE(value);
}


TEST(check_ptr, non_nullptr)
{
  const char *ptr = "test";
  EXPECT_EQ(ptr, sal_check_ptr(ptr));
}


TEST(check_ptr, nullptr)
{
  const char *ptr = nullptr, *checked_ptr = "test";

#if !defined(NDEBUG)
  EXPECT_THROW(checked_ptr = sal_check_ptr(ptr), std::logic_error);
  EXPECT_NE(nullptr, checked_ptr);
#else
  EXPECT_NO_THROW(checked_ptr = sal_check_ptr(ptr));
  EXPECT_EQ(nullptr, checked_ptr);
#endif
}


TEST(check_ptr, nullptr_t)
{
  int dummy;
  auto *p = static_cast<void *>(&dummy);

#if !defined(NDEBUG)
  EXPECT_THROW(p = sal_check_ptr(nullptr), std::logic_error);
  EXPECT_NE(nullptr, p);
#else
  EXPECT_NO_THROW(p = sal_check_ptr(nullptr));
  EXPECT_EQ(nullptr, p);
#endif
}
