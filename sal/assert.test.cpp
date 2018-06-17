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


TEST(check_ptr, raw_pointer)
{
  const char *ptr = "test";
  EXPECT_EQ(ptr, sal_check_ptr(ptr));

  ptr = nullptr;
#if !defined(NDEBUG)
  EXPECT_THROW(sal_check_ptr(ptr), std::logic_error);
#else
  EXPECT_EQ(nullptr, sal_check_ptr(ptr));
#endif
}


TEST(check_ptr, unique_ptr)
{
  auto ptr = std::make_unique<int>(1);
  EXPECT_EQ(ptr.get(), sal_check_ptr(ptr));

  ptr.reset();
#if !defined(NDEBUG)
  EXPECT_THROW(sal_check_ptr(ptr), std::logic_error);
#else
  EXPECT_EQ(nullptr, sal_check_ptr(ptr));
#endif
}


TEST(check_ptr, shared_ptr)
{
  auto ptr = std::make_shared<int>(1);
  EXPECT_EQ(ptr.get(), sal_check_ptr(ptr));

  ptr.reset();
#if !defined(NDEBUG)
  EXPECT_THROW(sal_check_ptr(ptr), std::logic_error);
#else
  EXPECT_EQ(nullptr, sal_check_ptr(ptr));
#endif
}


TEST(check_ptr, nullptr_t)
{
#if !defined(NDEBUG)
  EXPECT_THROW(sal_check_ptr(nullptr), std::logic_error);
#else
  EXPECT_EQ(nullptr, sal_check_ptr(nullptr));
#endif
}
