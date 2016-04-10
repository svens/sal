#include <sal/assert.hpp>
#include <sal/common.test.hpp>


TEST(expect, true)
{
#if !defined(SAL_RELEASE)
  EXPECT_NO_THROW(sal_expect(true));
#endif
}


TEST(expect, false)
{
#if !defined(SAL_RELEASE)
  EXPECT_THROW(sal_expect(false), std::logic_error);
#endif
}


TEST(ensure, true)
{
#if !defined(SAL_RELEASE)
  EXPECT_NO_THROW(sal_ensure(true));
#endif
}


TEST(ensure, false)
{
#if !defined(SAL_RELEASE)
  EXPECT_THROW(sal_ensure(false), std::logic_error);
#endif
}


TEST(check_ptr, true)
{
#if !defined(SAL_RELEASE)
  const char *ptr = "test";
  EXPECT_EQ(ptr, sal_check_ptr(ptr));
#endif
}


TEST(check_ptr, false)
{
#if !defined(SAL_RELEASE)
  EXPECT_THROW(sal_check_ptr(nullptr), std::logic_error);

  const char *ptr = nullptr;
  EXPECT_THROW(sal_check_ptr(ptr), std::logic_error);
#endif
}
