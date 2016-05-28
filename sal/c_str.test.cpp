#include <sal/c_str.hpp>
#include <sal/fmtval.hpp>
#include <sal/common.test.hpp>


namespace {


using c_str = sal_test::fixture;


constexpr size_t size = 256;


TEST_F(c_str, ctor)
{
  sal::c_str<size> c_str;
  ASSERT_TRUE(c_str.good());
  EXPECT_TRUE(c_str.empty());
  EXPECT_EQ(0U, c_str.size());
  EXPECT_EQ(size, c_str.max_size());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, copy_ctor_empty)
{
  sal::c_str<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  auto c_str = expected;
  ASSERT_TRUE(c_str.good());
  EXPECT_TRUE(c_str.empty());
  EXPECT_EQ(expected.size(), c_str.size());
  EXPECT_EQ(expected.max_size(), c_str.max_size());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, copy_ctor_different_size_empty)
{
  sal::c_str<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  sal::c_str<expected.max_size() + 1> c_str = expected;
  ASSERT_TRUE(c_str.good());
  EXPECT_TRUE(c_str.empty());
  EXPECT_EQ(0U, c_str.size());
  EXPECT_EQ(expected.max_size() + 1, c_str.max_size());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, copy_ctor_non_empty)
{
  sal::c_str<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  auto c_str = expected;
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(expected.size(), c_str.size());
  EXPECT_EQ(expected.max_size(), c_str.max_size());
  EXPECT_STREQ(expected.get(), c_str.get());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, copy_ctor_different_size_non_empty)
{
  sal::c_str<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  sal::c_str<expected.max_size() + 1> c_str = expected;
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(expected.size(), c_str.size());
  EXPECT_EQ(expected.max_size() + 1, c_str.max_size());
  EXPECT_STREQ(expected.get(), c_str.get());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, copy_ctor_invalid)
{
  sal::c_str<4> expected;
  expected << "1234" << "abcd";
  EXPECT_FALSE(expected.good());
  EXPECT_FALSE(expected.empty());
  EXPECT_EQ(8U, expected.size());
  EXPECT_STREQ("1234", expected.get());

  auto c_str = expected;
  EXPECT_FALSE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(8U, c_str.size());
  EXPECT_EQ('\0', *c_str.begin());
}


TEST_F(c_str, copy_assign_empty)
{
  sal::c_str<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  decltype(expected) c_str;
  c_str = expected;
  ASSERT_TRUE(c_str.good());
  EXPECT_TRUE(c_str.empty());
  EXPECT_EQ(expected.size(), c_str.size());
  EXPECT_EQ(expected.max_size(), c_str.max_size());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, copy_assign_different_size_empty)
{
  sal::c_str<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  sal::c_str<expected.max_size() + 1> c_str;
  c_str = expected;
  ASSERT_TRUE(c_str.good());
  EXPECT_TRUE(c_str.empty());
  EXPECT_EQ(expected.size(), c_str.size());
  EXPECT_EQ(expected.max_size() + 1, c_str.max_size());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, copy_assign_non_empty)
{
  sal::c_str<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  decltype(expected) c_str;
  c_str = expected;
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(expected.size(), c_str.size());
  EXPECT_EQ(expected.max_size(), c_str.max_size());
  EXPECT_STREQ(expected.get(), c_str.get());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, copy_assign_different_size_non_empty)
{
  sal::c_str<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  sal::c_str<expected.max_size() + 1> c_str;
  c_str = expected;
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(expected.size(), c_str.size());
  EXPECT_EQ(expected.max_size() + 1, c_str.max_size());
  EXPECT_STREQ(expected.get(), c_str.get());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, copy_assign_invalid)
{
  sal::c_str<4> expected;
  expected << "1234" << "abcd";
  EXPECT_FALSE(expected.good());
  EXPECT_FALSE(expected.empty());
  EXPECT_EQ(8U, expected.size());
  EXPECT_STREQ("1234", expected.get());

  decltype(expected) c_str;
  c_str = expected;
  EXPECT_FALSE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(8U, c_str.size());
  EXPECT_EQ('\0', *c_str.begin());
}


TEST_F(c_str, iterator)
{
  sal::c_str<size> c_str;
  EXPECT_EQ(c_str.begin(), c_str.end());
  EXPECT_EQ(c_str.cbegin(), c_str.cend());

  c_str << case_name;
  EXPECT_NE(c_str.begin(), c_str.end());
  EXPECT_NE(c_str.cbegin(), c_str.cend());

  EXPECT_EQ(c_str.size(),
    static_cast<size_t>(std::distance(c_str.begin(), c_str.end()))
  );
  EXPECT_EQ(c_str.size(),
    static_cast<size_t>(std::distance(c_str.cbegin(), c_str.cend()))
  );
}


TEST_F(c_str, data)
{
  sal::c_str<size> c_str;
  EXPECT_EQ(static_cast<void *>(&c_str), c_str.data());
}


TEST_F(c_str, front)
{
  sal::c_str<size> c_str;
  c_str << case_name;
  EXPECT_EQ(case_name.front(), c_str.front());
}


TEST_F(c_str, back)
{
  sal::c_str<size> c_str;
  c_str << case_name;
  EXPECT_EQ(case_name.back(), c_str.back());
}


TEST_F(c_str, index)
{
  sal::c_str<size> c_str;
  c_str << case_name;
  for (auto i = 0U;  i < case_name.size();  ++i)
  {
    EXPECT_EQ(case_name[i], c_str[i]);
  }
}


TEST_F(c_str, str)
{
  sal::c_str<size> c_str;
  c_str << case_name;
  EXPECT_EQ(case_name, c_str.str());
  EXPECT_STREQ(case_name.c_str(), c_str.get());
}


TEST_F(c_str, insert_single)
{
  sal::c_str<4> c_str;
  c_str << "1234";
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(4U, c_str.size());
  EXPECT_STREQ("1234", c_str.get());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, insert_multiple)
{
  sal::c_str<4> c_str;

  c_str << "12";
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(2U, c_str.size());
  EXPECT_STREQ("12", c_str.get());
  EXPECT_EQ('\0', *c_str.end());

  c_str << "ab";
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(4U, c_str.size());
  EXPECT_STREQ("12ab", c_str.get());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, insert_single_overflow)
{
  sal::c_str<4> c_str;

  c_str << "12345";
  EXPECT_FALSE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(5U, c_str.size());

  c_str.restore();
  ASSERT_TRUE(c_str.good());
  EXPECT_TRUE(c_str.empty());
  EXPECT_EQ(0U, c_str.size());
}


TEST_F(c_str, insert_multiple_overflow)
{
  sal::c_str<4> c_str;

  c_str << "123";
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(3U, c_str.size());
  EXPECT_STREQ("123", c_str.get());
  EXPECT_EQ('\0', *c_str.end());

  c_str << "4";
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(4U, c_str.size());
  EXPECT_STREQ("1234", c_str.get());
  EXPECT_EQ('\0', *c_str.end());

  c_str << "56";
  EXPECT_FALSE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(6U, c_str.size());

  c_str.restore();
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(4U, c_str.size());
  EXPECT_STREQ("1234", c_str.get());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, insert_single_clear)
{
  sal::c_str<4> c_str;

  c_str << "1234";
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(4U, c_str.size());
  EXPECT_STREQ("1234", c_str.get());
  EXPECT_EQ('\0', *c_str.end());

  c_str.reset();
  ASSERT_TRUE(c_str.good());
  EXPECT_TRUE(c_str.empty());
  EXPECT_EQ(0U, c_str.size());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, insert_multiple_clear)
{
  sal::c_str<4> c_str;

  c_str << "123";
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(3U, c_str.size());
  EXPECT_STREQ("123", c_str.get());
  EXPECT_EQ('\0', *c_str.end());

  c_str << "4";
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(4U, c_str.size());
  EXPECT_EQ('\0', *c_str.end());

  c_str << "56";
  EXPECT_FALSE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(6U, c_str.size());

  c_str.reset();
  ASSERT_TRUE(c_str.good());
  EXPECT_TRUE(c_str.empty());
  EXPECT_EQ(0U, c_str.size());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, insert_c_str)
{
  sal::c_str<4> c_str, another;
  c_str << "12";
  another << "34";
  c_str << another;

  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(4U, c_str.size());
  EXPECT_STREQ("1234", c_str.get());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, insert_self)
{
  sal::c_str<4> c_str;
  c_str << "12";
  c_str << c_str;

  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(4U, c_str.size());
  EXPECT_STREQ("1212", c_str.get());
  EXPECT_EQ('\0', *c_str.end());
}


TEST_F(c_str, insert_self_overflow)
{
  sal::c_str<4> c_str;
  c_str << "12";

  c_str << c_str;
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(4U, c_str.size());
  EXPECT_STREQ("1212", c_str.get());
  EXPECT_EQ('\0', *c_str.end());

  c_str << c_str;
  EXPECT_FALSE(c_str.good());
  EXPECT_EQ(8U, c_str.size());
  EXPECT_FALSE(c_str.empty());
}


TEST_F(c_str, insert_ostream)
{
  sal::c_str<4> c_str;
  c_str << "1234";
  ASSERT_TRUE(c_str.good());
  EXPECT_FALSE(c_str.empty());
  EXPECT_EQ(4U, c_str.size());
  EXPECT_STREQ("1234", c_str.get());
  EXPECT_EQ('\0', *c_str.end());

  std::ostringstream oss;
  oss << c_str;
  EXPECT_EQ("1234", oss.str());
}


TEST_F(c_str, print)
{
  sal::c_str<32> c_str;
  sal::print(c_str, case_name, 12, 34);
  ASSERT_TRUE(c_str.good());
  EXPECT_EQ(case_name + "1234", c_str.get());
}


TEST_F(c_str, print_overflow)
{
  sal::c_str<4> c_str;
  sal::print(c_str, 12, 34);
  ASSERT_TRUE(c_str.good());
  EXPECT_STREQ("1234", c_str.get());

  sal::print(c_str, 56);
  EXPECT_FALSE(c_str.good());
  EXPECT_EQ(6U, c_str.size());
}


TEST_F(c_str, fmt_v)
{
  sal::c_str<4> c_str;
  c_str << "123";

  char data[8];
  auto end = sal::fmt_v(c_str, data);
  EXPECT_EQ("123", std::string(data, end));
}


} // namespace
