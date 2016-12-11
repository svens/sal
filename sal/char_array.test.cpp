#include <sal/char_array.hpp>
#include <sal/common.test.hpp>


namespace {


constexpr size_t size = 256;


struct char_array
  : public sal_test::fixture
{
  sal::char_array_t<256> chars;
  std::string exact, overflow;

  char_array ()
    : exact(chars.max_size(), '.')
    , overflow(exact + exact)
  {}
};


TEST_F(char_array, ctor)
{
  ASSERT_TRUE(chars.good());

  EXPECT_TRUE(bool(chars));
  EXPECT_FALSE(chars.bad());
  EXPECT_FALSE(chars.full());
  EXPECT_TRUE(chars.empty());

  EXPECT_EQ(0U, chars.size());
  EXPECT_EQ(size, chars.max_size());
  EXPECT_EQ(size, chars.available());

  EXPECT_STREQ("", chars.c_str());
}


TEST_F(char_array, ctor_assign)
{
  ASSERT_TRUE(bool(chars << case_name));
  auto a{chars};
  EXPECT_EQ(case_name, a.c_str());
}


TEST_F(char_array, ctor_assign_empty)
{
  auto a{chars};
  EXPECT_EQ("", a.to_string());
}


TEST_F(char_array, ctor_assign_bad)
{
  EXPECT_TRUE(bool(chars << exact));
  EXPECT_FALSE(bool(chars << case_name));
  EXPECT_TRUE(chars.bad());

  auto a{chars};
  EXPECT_TRUE(a.good());
  EXPECT_EQ(exact, a.c_str());
}


TEST_F(char_array, ctor_assign_smaller)
{
  ASSERT_TRUE(bool(chars << case_name));
  sal::char_array_t<size / 2> a{chars};
  EXPECT_EQ(case_name, a.c_str());
}


TEST_F(char_array, ctor_assign_smaller_empty)
{
  sal::char_array_t<size / 2> a{chars};
  EXPECT_EQ("", a.to_string());
}


TEST_F(char_array, ctor_assign_smaller_bad)
{
  EXPECT_TRUE(bool(chars << exact));
  EXPECT_FALSE(bool(chars << case_name));
  EXPECT_TRUE(chars.bad());

  sal::char_array_t<size / 2> a{chars};
  EXPECT_TRUE(a.good());
  std::string half_exact(exact, exact.size() / 2);
  EXPECT_EQ(half_exact, a.c_str());
}


TEST_F(char_array, ctor_assign_bigger)
{
  ASSERT_TRUE(bool(chars << case_name));
  sal::char_array_t<size * 2> a{chars};
  EXPECT_EQ(case_name, a.c_str());
}


TEST_F(char_array, ctor_assign_bigger_empty)
{
  sal::char_array_t<size * 2> a{chars};
  EXPECT_EQ("", a.to_string());
}


TEST_F(char_array, ctor_assign_bigger_bad)
{
  EXPECT_TRUE(bool(chars << exact));
  EXPECT_FALSE(bool(chars << case_name));
  EXPECT_TRUE(chars.bad());

  sal::char_array_t<size * 2> a{chars};
  EXPECT_TRUE(a.good());
  EXPECT_EQ(exact, a.c_str());
}


TEST_F(char_array, copy_assign)
{
  ASSERT_TRUE(bool(chars << case_name));
  decltype(chars) a;
  a = chars;
  EXPECT_EQ(case_name, a.c_str());
}


TEST_F(char_array, copy_assign_empty)
{
  decltype(chars) a;
  a = chars;
  EXPECT_EQ("", a.to_string());
}


TEST_F(char_array, copy_assign_bad)
{
  EXPECT_TRUE(bool(chars << exact));
  EXPECT_FALSE(bool(chars << case_name));
  EXPECT_TRUE(chars.bad());

  decltype(chars) a;
  a = chars;
  EXPECT_TRUE(a.good());
  EXPECT_EQ(exact, a.c_str());
}


TEST_F(char_array, copy_assign_smaller)
{
  ASSERT_TRUE(bool(chars << case_name));
  sal::char_array_t<size / 2> a;
  a = chars;
  EXPECT_EQ(case_name, a.c_str());
}


TEST_F(char_array, copy_assign_smaller_empty)
{
  sal::char_array_t<size / 2> a;
  a = chars;
  EXPECT_EQ("", a.to_string());
}


TEST_F(char_array, copy_assign_smaller_bad)
{
  EXPECT_TRUE(bool(chars << exact));
  EXPECT_FALSE(bool(chars << case_name));
  EXPECT_TRUE(chars.bad());

  sal::char_array_t<size / 2> a;
  a = chars;
  EXPECT_TRUE(a.good());
  std::string half_exact(exact, exact.size() / 2);
  EXPECT_EQ(half_exact, a.c_str());
}


TEST_F(char_array, copy_assign_bigger)
{
  ASSERT_TRUE(bool(chars << case_name));
  sal::char_array_t<size * 2> a;
  a = chars;
  EXPECT_EQ(case_name, a.c_str());
}


TEST_F(char_array, copy_assign_bigger_empty)
{
  sal::char_array_t<size * 2> a;
  a = chars;
  EXPECT_EQ("", a.to_string());
}


TEST_F(char_array, copy_assign_bigger_bad)
{
  EXPECT_TRUE(bool(chars << exact));
  EXPECT_FALSE(bool(chars << case_name));
  EXPECT_TRUE(chars.bad());

  sal::char_array_t<size * 2> a;
  a = chars;
  EXPECT_TRUE(a.good());
  EXPECT_EQ(exact, a.c_str());
}


TEST_F(char_array, index)
{
  ASSERT_TRUE(bool(chars << case_name));
  for (auto i = 0U;  i < case_name.size();  ++i)
  {
    EXPECT_EQ(case_name[i], chars[i]);
  }
}


TEST_F(char_array, front)
{
  ASSERT_TRUE(bool(chars << case_name));
  EXPECT_EQ(case_name.front(), chars.front());
}


TEST_F(char_array, back)
{
  ASSERT_TRUE(bool(chars << case_name));
  EXPECT_EQ(case_name.back(), chars.back());
}


TEST_F(char_array, iterator)
{
  ASSERT_TRUE(bool(chars << case_name));
  EXPECT_EQ(chars.data(), chars.begin());
  EXPECT_EQ(chars.data() + case_name.size(), chars.end());
}


TEST_F(char_array, iterator_empty)
{
  EXPECT_EQ(chars.data(), chars.begin());
  EXPECT_EQ(chars.data(), chars.end());
}


TEST_F(char_array, iterator_full)
{
  ASSERT_TRUE(bool(chars << exact));
  EXPECT_EQ(chars.data(), chars.begin());
  EXPECT_EQ(chars.data() + exact.size(), chars.end());
}


TEST_F(char_array, iterator_bad)
{
  EXPECT_FALSE(bool(chars << overflow));
  EXPECT_EQ(chars.data(), chars.begin());
  EXPECT_EQ(chars.data() + overflow.size(), chars.end());
}


TEST_F(char_array, remove_suffix)
{
  EXPECT_TRUE(bool(chars << case_name));
  chars.remove_suffix(case_name.size() / 2);
  std::string expected(case_name, 0, case_name.size() / 2);
  EXPECT_EQ(expected, chars.c_str());
}


TEST_F(char_array, remove_suffix_before_begin)
{
  EXPECT_TRUE(bool(chars << case_name));
  chars.remove_suffix(case_name.size() * 2);
  EXPECT_EQ(0U, chars.size());
  EXPECT_EQ(chars.begin(), chars.end());
}


TEST_F(char_array, remove_suffix_from_bad)
{
  EXPECT_FALSE(bool(chars << overflow));
  EXPECT_TRUE(chars.bad());

  chars.remove_suffix(overflow.size() / 3);
  EXPECT_TRUE(chars.bad());

  chars.remove_suffix(overflow.size() / 3);
  EXPECT_TRUE(chars.good());
}


TEST_F(char_array, mark_and_revert)
{
  ASSERT_TRUE(bool(chars << case_name));
  EXPECT_EQ(case_name, chars.c_str());

  auto mark = chars.mark();
  ASSERT_TRUE(bool(chars << case_name));
  EXPECT_EQ(case_name + case_name, chars.c_str());

  chars.revert(mark);
  EXPECT_EQ(case_name, chars.c_str());
}


TEST_F(char_array, mark_and_revert_from_bad)
{
  ASSERT_TRUE(bool(chars << case_name));
  EXPECT_EQ(case_name, chars.c_str());

  auto mark = chars.mark();
  ASSERT_FALSE(bool(chars << exact));
  EXPECT_TRUE(chars.bad());

  chars.revert(mark);
  ASSERT_TRUE(chars.good());
  EXPECT_EQ(case_name, chars.c_str());
}


TEST_F(char_array, reset)
{
  ASSERT_TRUE(bool(chars << case_name));
  EXPECT_EQ(case_name, chars.c_str());
  EXPECT_EQ(case_name.size(), chars.size());

  chars.reset();
  EXPECT_TRUE(chars.empty());
  EXPECT_STREQ("", chars.c_str());
}


TEST_F(char_array, reset_empty)
{
  chars.reset();
  EXPECT_TRUE(chars.empty());
  EXPECT_STREQ("", chars.c_str());
}


TEST_F(char_array, reset_full)
{
  ASSERT_TRUE(bool(chars << exact));
  EXPECT_EQ(exact, chars.c_str());
  EXPECT_EQ(exact.size(), chars.size());

  chars.reset();
  EXPECT_TRUE(chars.empty());
  EXPECT_STREQ("", chars.c_str());
}


TEST_F(char_array, reset_bad)
{
  EXPECT_FALSE(bool(chars << overflow));
  EXPECT_TRUE(chars.bad());

  chars.reset();
  ASSERT_TRUE(chars.good());
  ASSERT_FALSE(chars.bad());
  EXPECT_TRUE(chars.empty());
  EXPECT_STREQ("", chars.c_str());
}


TEST_F(char_array, insert)
{
  ASSERT_TRUE(bool(chars << case_name));
  EXPECT_TRUE(chars.good());
  EXPECT_EQ(case_name, chars.c_str());
}


TEST_F(char_array, insert_full)
{
  ASSERT_TRUE(bool(chars << exact));
  EXPECT_TRUE(chars.full());
  EXPECT_EQ(exact, chars.c_str());
}


TEST_F(char_array, insert_overflow)
{
  EXPECT_FALSE(bool(chars << overflow));
  EXPECT_TRUE(chars.bad());
}


TEST_F(char_array, insert_char_array)
{
  sal::char_array_t<2 * size> a;
  EXPECT_TRUE(bool(a << case_name));
  EXPECT_TRUE(bool(chars << a));
  EXPECT_EQ(case_name, chars.c_str());
}


TEST_F(char_array, insert_self)
{
  EXPECT_TRUE(bool(chars << case_name));
  EXPECT_TRUE(bool(chars << chars));
  EXPECT_EQ(case_name + case_name, chars.c_str());
}


struct user_defined_t
{
  static std::string name ()
  {
    return "user_defined_t";
  }
};


sal::memory_writer_t &operator<< (sal::memory_writer_t &writer,
  const user_defined_t &) noexcept
{
  return (writer << user_defined_t::name());
}


TEST_F(char_array, insert_user_defined_type)
{
  user_defined_t t;
  EXPECT_TRUE(bool(chars << t));
  EXPECT_EQ(user_defined_t::name(), chars.c_str());
}


TEST_F(char_array, print)
{
  ASSERT_TRUE(bool(chars.print(case_name)));
  EXPECT_TRUE(chars.good());
  EXPECT_EQ(case_name, chars.c_str());
}


TEST_F(char_array, print_full)
{
  ASSERT_TRUE(bool(chars.print(exact)));
  EXPECT_TRUE(chars.full());
  EXPECT_EQ(exact, chars.c_str());
}


TEST_F(char_array, print_overflow)
{
  EXPECT_FALSE(bool(chars.print(overflow)));
  EXPECT_TRUE(chars.bad());
}


TEST_F(char_array, print_char_array)
{
  sal::char_array_t<2 * size> a;
  EXPECT_TRUE(bool(a << case_name));
  EXPECT_TRUE(bool(chars.print(a)));
  EXPECT_EQ(case_name, chars.c_str());
}


TEST_F(char_array, print_self)
{
  EXPECT_TRUE(bool(chars << case_name));
  EXPECT_TRUE(bool(chars.print(chars)));
  EXPECT_EQ(case_name + case_name, chars.c_str());
}


TEST_F(char_array, print_user_defined_type)
{
  user_defined_t t;
  EXPECT_TRUE(bool(chars.print(t)));
  EXPECT_EQ(user_defined_t::name(), chars.c_str());
}


TEST_F(char_array, to_string)
{
  ASSERT_TRUE(bool(chars << case_name));
  EXPECT_TRUE(chars.good());
  EXPECT_EQ(case_name, chars.to_string());
}


TEST_F(char_array, to_string_empty)
{
  ASSERT_TRUE(chars.good());
  EXPECT_EQ("", chars.to_string());
}


TEST_F(char_array, to_string_full)
{
  ASSERT_TRUE(bool(chars << exact));
  EXPECT_TRUE(chars.good());
  EXPECT_TRUE(chars.full());
  EXPECT_EQ(exact, chars.to_string());
}


} // namespace
