#include <sal/str.hpp>
#include <sal/fmt.hpp>
#include <sal/common.test.hpp>


namespace {


using str = sal_test::fixture;


constexpr size_t size = 256;


TEST_F(str, ctor)
{
  sal::str_t<size> str;
  ASSERT_TRUE(str.good());
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(0U, str.size());
  EXPECT_EQ(size, str.max_size());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, copy_ctor_empty)
{
  sal::str_t<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  auto str = expected;
  ASSERT_TRUE(str.good());
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(expected.size(), str.size());
  EXPECT_EQ(expected.max_size(), str.max_size());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, copy_ctor_different_size_empty)
{
  sal::str_t<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  sal::str_t<expected.max_size() + 1> str = expected;
  ASSERT_TRUE(str.good());
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(0U, str.size());
  EXPECT_EQ(expected.max_size() + 1, str.max_size());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, copy_ctor_non_empty)
{
  sal::str_t<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  auto str = expected;
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(expected.size(), str.size());
  EXPECT_EQ(expected.max_size(), str.max_size());
  EXPECT_STREQ(expected.get(), str.get());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, copy_ctor_different_size_non_empty)
{
  sal::str_t<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  sal::str_t<expected.max_size() + 1> str = expected;
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(expected.size(), str.size());
  EXPECT_EQ(expected.max_size() + 1, str.max_size());
  EXPECT_STREQ(expected.get(), str.get());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, copy_ctor_invalid)
{
  sal::str_t<4> expected;
  expected << "1234" << "abcd";
  EXPECT_FALSE(expected.good());
  EXPECT_FALSE(expected.empty());
  EXPECT_EQ(8U, expected.size());
  EXPECT_STREQ("1234", expected.get());

  auto str = expected;
  EXPECT_FALSE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(8U, str.size());
  EXPECT_EQ('\0', *str.begin());
}


TEST_F(str, copy_assign_empty)
{
  sal::str_t<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  decltype(expected) str;
  str = expected;
  ASSERT_TRUE(str.good());
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(expected.size(), str.size());
  EXPECT_EQ(expected.max_size(), str.max_size());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, copy_assign_different_size_empty)
{
  sal::str_t<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  sal::str_t<expected.max_size() + 1> str;
  str = expected;
  ASSERT_TRUE(str.good());
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(expected.size(), str.size());
  EXPECT_EQ(expected.max_size() + 1, str.max_size());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, copy_assign_non_empty)
{
  sal::str_t<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  decltype(expected) str;
  str = expected;
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(expected.size(), str.size());
  EXPECT_EQ(expected.max_size(), str.max_size());
  EXPECT_STREQ(expected.get(), str.get());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, copy_assign_different_size_non_empty)
{
  sal::str_t<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  sal::str_t<expected.max_size() + 1> str;
  str = expected;
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(expected.size(), str.size());
  EXPECT_EQ(expected.max_size() + 1, str.max_size());
  EXPECT_STREQ(expected.get(), str.get());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, copy_assign_invalid)
{
  sal::str_t<4> expected;
  expected << "1234" << "abcd";
  EXPECT_FALSE(expected.good());
  EXPECT_FALSE(expected.empty());
  EXPECT_EQ(8U, expected.size());
  EXPECT_STREQ("1234", expected.get());

  decltype(expected) str;
  str = expected;
  EXPECT_FALSE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(8U, str.size());
  EXPECT_EQ('\0', *str.begin());
}


TEST_F(str, iterator)
{
  sal::str_t<size> str;
  EXPECT_EQ(str.begin(), str.end());
  EXPECT_EQ(str.cbegin(), str.cend());

  str << case_name;
  EXPECT_NE(str.begin(), str.end());
  EXPECT_NE(str.cbegin(), str.cend());

  EXPECT_EQ(str.size(),
    static_cast<size_t>(std::distance(str.begin(), str.end()))
  );
  EXPECT_EQ(str.size(),
    static_cast<size_t>(std::distance(str.cbegin(), str.cend()))
  );
}


TEST_F(str, data)
{
  sal::str_t<size> str;
  EXPECT_EQ(static_cast<void *>(&str), str.data());
}


TEST_F(str, front)
{
  sal::str_t<size> str;
  str << case_name;
  EXPECT_EQ(case_name.front(), str.front());
}


TEST_F(str, back)
{
  sal::str_t<size> str;
  str << case_name;
  EXPECT_EQ(case_name.back(), str.back());
}


TEST_F(str, index)
{
  sal::str_t<size> str;
  str << case_name;
  for (auto i = 0U;  i < case_name.size();  ++i)
  {
    EXPECT_EQ(case_name[i], str[i]);
  }
}


TEST_F(str, remove_suffix)
{
  sal::str_t<size> str;

  str << "help";
  EXPECT_STREQ("help", str.get());

  str.remove_suffix(2);
  EXPECT_STREQ("he", str.get());

  str << "llo";
  EXPECT_STREQ("hello", str.get());
}


TEST_F(str, remove_suffix_underflow)
{
  sal::str_t<size> str;
  str << case_name;
  EXPECT_STREQ(case_name.c_str(), str.get());

  str.remove_suffix(2 * case_name.size());
  EXPECT_EQ(0U, str.size());
  EXPECT_STREQ("", str.get());
}


TEST_F(str, to_string)
{
  sal::str_t<size> str;
  str << case_name;
  EXPECT_EQ(case_name, sal::to_string(str));
  EXPECT_STREQ(case_name.c_str(), str.get());
}


TEST_F(str, insert_single)
{
  sal::str_t<4> str;
  str << "1234";
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(4U, str.size());
  EXPECT_STREQ("1234", str.get());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, insert_multiple)
{
  sal::str_t<4> str;

  str << "12";
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(2U, str.size());
  EXPECT_STREQ("12", str.get());
  EXPECT_EQ('\0', *str.end());

  str << "ab";
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(4U, str.size());
  EXPECT_STREQ("12ab", str.get());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, insert_single_overflow)
{
  sal::str_t<4> str;

  str << "12345";
  EXPECT_FALSE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(5U, str.size());

  str.restore();
  ASSERT_TRUE(str.good());
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(0U, str.size());
}


TEST_F(str, insert_multiple_overflow)
{
  sal::str_t<4> str;

  str << "123";
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(3U, str.size());
  EXPECT_STREQ("123", str.get());
  EXPECT_EQ('\0', *str.end());

  str << "4";
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(4U, str.size());
  EXPECT_STREQ("1234", str.get());
  EXPECT_EQ('\0', *str.end());

  str << "56";
  EXPECT_FALSE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(6U, str.size());

  str.restore();
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(4U, str.size());
  EXPECT_STREQ("1234", str.get());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, insert_single_clear)
{
  sal::str_t<4> str;

  str << "1234";
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(4U, str.size());
  EXPECT_STREQ("1234", str.get());
  EXPECT_EQ('\0', *str.end());

  str.reset();
  ASSERT_TRUE(str.good());
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(0U, str.size());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, insert_multiple_clear)
{
  sal::str_t<4> str;

  str << "123";
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(3U, str.size());
  EXPECT_STREQ("123", str.get());
  EXPECT_EQ('\0', *str.end());

  str << "4";
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(4U, str.size());
  EXPECT_EQ('\0', *str.end());

  str << "56";
  EXPECT_FALSE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(6U, str.size());

  str.reset();
  ASSERT_TRUE(str.good());
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(0U, str.size());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, insert_str)
{
  sal::str_t<4> str, another;
  str << "12";
  another << "34";
  str << another;

  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(4U, str.size());
  EXPECT_STREQ("1234", str.get());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, insert_self)
{
  sal::str_t<4> str;
  str << "12";
  str << str;

  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(4U, str.size());
  EXPECT_STREQ("1212", str.get());
  EXPECT_EQ('\0', *str.end());
}


TEST_F(str, insert_self_overflow)
{
  sal::str_t<4> str;
  str << "12";

  str << str;
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(4U, str.size());
  EXPECT_STREQ("1212", str.get());
  EXPECT_EQ('\0', *str.end());

  str << str;
  EXPECT_FALSE(str.good());
  EXPECT_EQ(8U, str.size());
  EXPECT_FALSE(str.empty());
}


TEST_F(str, insert_ostream)
{
  sal::str_t<4> str;
  str << "1234";
  ASSERT_TRUE(str.good());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(4U, str.size());
  EXPECT_STREQ("1234", str.get());
  EXPECT_EQ('\0', *str.end());

  std::ostringstream oss;
  oss << str;
  EXPECT_EQ("1234", oss.str());
}


TEST_F(str, print)
{
  sal::str_t<32> str;
  sal::print(str, case_name, 12, 34);
  ASSERT_TRUE(str.good());
  EXPECT_EQ(case_name + "1234", str.get());
}


TEST_F(str, print_overflow)
{
  sal::str_t<4> str;
  sal::print(str, 12, 34);
  ASSERT_TRUE(str.good());
  EXPECT_STREQ("1234", str.get());

  sal::print(str, 56);
  EXPECT_FALSE(str.good());
  EXPECT_EQ(6U, str.size());
}


TEST_F(str, fmt)
{
  sal::str_t<4> str;
  str << "123";

  char data[8];
  auto end = sal::fmt(str, data);
  EXPECT_EQ("123", std::string(data, end));
}


} // namespace
