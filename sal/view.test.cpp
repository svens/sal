#include <sal/view.hpp>
#include <sal/fmtval.hpp>
#include <sal/common.test.hpp>


namespace {


using view = sal_test::fixture;


constexpr size_t size = 256;


TEST_F(view, ctor)
{
  sal::view<size> view;
  ASSERT_TRUE(view.good());
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(0U, view.size());
  EXPECT_EQ(size, view.max_size());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, copy_ctor_empty)
{
  sal::view<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  auto view = expected;
  ASSERT_TRUE(view.good());
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(expected.size(), view.size());
  EXPECT_EQ(expected.max_size(), view.max_size());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, copy_ctor_different_size_empty)
{
  sal::view<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  sal::view<expected.max_size() + 1> view = expected;
  ASSERT_TRUE(view.good());
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(0U, view.size());
  EXPECT_EQ(expected.max_size() + 1, view.max_size());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, copy_ctor_non_empty)
{
  sal::view<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  auto view = expected;
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(expected.size(), view.size());
  EXPECT_EQ(expected.max_size(), view.max_size());
  EXPECT_STREQ(expected.c_str(), view.c_str());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, copy_ctor_different_size_non_empty)
{
  sal::view<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  sal::view<expected.max_size() + 1> view = expected;
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(expected.size(), view.size());
  EXPECT_EQ(expected.max_size() + 1, view.max_size());
  EXPECT_STREQ(expected.c_str(), view.c_str());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, copy_ctor_invalid)
{
  sal::view<4> expected;
  expected << "1234" << "abcd";
  EXPECT_FALSE(expected.good());
  EXPECT_FALSE(expected.empty());
  EXPECT_EQ(8U, expected.size());
  EXPECT_STREQ("1234", expected.c_str());

  auto view = expected;
  EXPECT_FALSE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(8U, view.size());
  EXPECT_EQ('\0', *view.begin());
}


TEST_F(view, copy_assign_empty)
{
  sal::view<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  decltype(expected) view;
  view = expected;
  ASSERT_TRUE(view.good());
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(expected.size(), view.size());
  EXPECT_EQ(expected.max_size(), view.max_size());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, copy_assign_different_size_empty)
{
  sal::view<size> expected;
  ASSERT_TRUE(expected.good());
  EXPECT_TRUE(expected.empty());

  sal::view<expected.max_size() + 1> view;
  view = expected;
  ASSERT_TRUE(view.good());
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(expected.size(), view.size());
  EXPECT_EQ(expected.max_size() + 1, view.max_size());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, copy_assign_non_empty)
{
  sal::view<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  decltype(expected) view;
  view = expected;
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(expected.size(), view.size());
  EXPECT_EQ(expected.max_size(), view.max_size());
  EXPECT_STREQ(expected.c_str(), view.c_str());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, copy_assign_different_size_non_empty)
{
  sal::view<size> expected;
  expected << case_name;
  ASSERT_TRUE(expected.good());
  EXPECT_FALSE(expected.empty());

  sal::view<expected.max_size() + 1> view;
  view = expected;
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(expected.size(), view.size());
  EXPECT_EQ(expected.max_size() + 1, view.max_size());
  EXPECT_STREQ(expected.c_str(), view.c_str());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, copy_assign_invalid)
{
  sal::view<4> expected;
  expected << "1234" << "abcd";
  EXPECT_FALSE(expected.good());
  EXPECT_FALSE(expected.empty());
  EXPECT_EQ(8U, expected.size());
  EXPECT_STREQ("1234", expected.c_str());

  decltype(expected) view;
  view = expected;
  EXPECT_FALSE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(8U, view.size());
  EXPECT_EQ('\0', *view.begin());
}


TEST_F(view, iterator)
{
  sal::view<size> view;
  EXPECT_EQ(view.begin(), view.end());
  EXPECT_EQ(view.cbegin(), view.cend());

  view << case_name;
  EXPECT_NE(view.begin(), view.end());
  EXPECT_NE(view.cbegin(), view.cend());

  EXPECT_EQ(view.size(),
    static_cast<size_t>(std::distance(view.begin(), view.end()))
  );
  EXPECT_EQ(view.size(),
    static_cast<size_t>(std::distance(view.cbegin(), view.cend()))
  );
}


TEST_F(view, data)
{
  sal::view<size> view;
  EXPECT_EQ(static_cast<void *>(&view), view.data());
}


TEST_F(view, front)
{
  sal::view<size> view;
  view << case_name;
  EXPECT_EQ(case_name.front(), view.front());
}


TEST_F(view, back)
{
  sal::view<size> view;
  view << case_name;
  EXPECT_EQ(case_name.back(), view.back());
}


TEST_F(view, index)
{
  sal::view<size> view;
  view << case_name;
  for (auto i = 0U;  i < case_name.size();  ++i)
  {
    EXPECT_EQ(case_name[i], view[i]);
  }
}


TEST_F(view, str)
{
  sal::view<size> view;
  view << case_name;
  EXPECT_EQ(case_name, view.str());
  EXPECT_STREQ(case_name.c_str(), view.c_str());
}


TEST_F(view, insert_single)
{
  sal::view<4> view;
  view << "1234";
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(4U, view.size());
  EXPECT_STREQ("1234", view.c_str());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, insert_multiple)
{
  sal::view<4> view;

  view << "12";
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(2U, view.size());
  EXPECT_STREQ("12", view.c_str());
  EXPECT_EQ('\0', *view.end());

  view << "ab";
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(4U, view.size());
  EXPECT_STREQ("12ab", view.c_str());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, insert_single_overflow)
{
  sal::view<4> view;

  view << "12345";
  EXPECT_FALSE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(5U, view.size());

  view.restore();
  ASSERT_TRUE(view.good());
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(0U, view.size());
}


TEST_F(view, insert_multiple_overflow)
{
  sal::view<4> view;

  view << "123";
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(3U, view.size());
  EXPECT_STREQ("123", view.c_str());
  EXPECT_EQ('\0', *view.end());

  view << "4";
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(4U, view.size());
  EXPECT_STREQ("1234", view.c_str());
  EXPECT_EQ('\0', *view.end());

  view << "56";
  EXPECT_FALSE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(6U, view.size());

  view.restore();
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(4U, view.size());
  EXPECT_STREQ("1234", view.c_str());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, insert_single_clear)
{
  sal::view<4> view;

  view << "1234";
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(4U, view.size());
  EXPECT_STREQ("1234", view.c_str());
  EXPECT_EQ('\0', *view.end());

  view.reset();
  ASSERT_TRUE(view.good());
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(0U, view.size());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, insert_multiple_clear)
{
  sal::view<4> view;

  view << "123";
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(3U, view.size());
  EXPECT_STREQ("123", view.c_str());
  EXPECT_EQ('\0', *view.end());

  view << "4";
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(4U, view.size());
  EXPECT_EQ('\0', *view.end());

  view << "56";
  EXPECT_FALSE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(6U, view.size());

  view.reset();
  ASSERT_TRUE(view.good());
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(0U, view.size());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, insert_view)
{
  sal::view<4> view, another;
  view << "12";
  another << "34";
  view << another;

  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(4U, view.size());
  EXPECT_STREQ("1234", view.c_str());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, insert_self)
{
  sal::view<4> view;
  view << "12";
  view << view;

  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(4U, view.size());
  EXPECT_STREQ("1212", view.c_str());
  EXPECT_EQ('\0', *view.end());
}


TEST_F(view, insert_self_overflow)
{
  sal::view<4> view;
  view << "12";

  view << view;
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(4U, view.size());
  EXPECT_STREQ("1212", view.c_str());
  EXPECT_EQ('\0', *view.end());

  view << view;
  EXPECT_FALSE(view.good());
  EXPECT_EQ(8U, view.size());
  EXPECT_FALSE(view.empty());
}


TEST_F(view, insert_ostream)
{
  sal::view<4> view;
  view << "1234";
  ASSERT_TRUE(view.good());
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(4U, view.size());
  EXPECT_STREQ("1234", view.c_str());
  EXPECT_EQ('\0', *view.end());

  std::ostringstream oss;
  oss << view;
  EXPECT_EQ("1234", oss.str());
}


TEST_F(view, fmt_v)
{
  sal::view<4> view;
  view << "123";

  char data[8];
  auto end = sal::fmt_v(view, data);
  EXPECT_EQ("123", std::string(data, end));
}


} // namespace
