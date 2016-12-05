#include <sal/utility.hpp>
#include <sal/common.test.hpp>


namespace {


using utility = sal_test::fixture;


TEST_F(utility, copy)
{
  char src[] = "123", dest[2 * sizeof(src)] = "";
  auto end = sal::copy(dest, dest + sizeof(dest), src, src + sizeof(src));
  EXPECT_EQ(dest + sizeof(src), end);
  EXPECT_STREQ(src, dest);
}


TEST_F(utility, copy_exact)
{
  char src[] = "123", dest[sizeof(src)] = "";
  auto end = sal::copy(dest, dest + sizeof(dest), src, src + sizeof(src));
  EXPECT_EQ(dest + sizeof(src), end);
  EXPECT_STREQ(src, dest);
}


TEST_F(utility, copy_overflow)
{
  char src[] = "123", dest[sizeof(src)] = "";
  auto end = sal::copy(dest, dest + sizeof(dest)/2, src, src + sizeof(src));
  EXPECT_EQ(dest + sizeof(src), end);
  EXPECT_STREQ("", dest);
}


TEST_F(utility, copy_n)
{
  char src[] = "123", dest[2 * sizeof(src)] = "";
  auto end = sal::copy(dest, dest + sizeof(dest), src);
  EXPECT_EQ(dest + sizeof(src), end);
  EXPECT_STREQ(src, dest);
}


TEST_F(utility, copy_n_exact)
{
  char src[] = "123", dest[sizeof(src)] = "";
  auto end = sal::copy(dest, dest + sizeof(dest), src);
  EXPECT_EQ(dest + sizeof(src), end);
  EXPECT_STREQ(src, dest);
}


TEST_F(utility, copy_n_overflow)
{
  char src[] = "123", dest[sizeof(src)] = "";
  auto end = sal::copy(dest, dest + sizeof(dest)/2, src);
  EXPECT_EQ(dest + sizeof(src), end);
  EXPECT_STREQ("", dest);
}


TEST_F(utility, c_str_bool_true)
{
  char dest[] = "................";
  auto end = sal::c_str(dest, dest + sizeof(dest), true);
  EXPECT_EQ(5U, end - dest);
  EXPECT_STREQ("true", dest);
}


TEST_F(utility, c_str_bool_true_exact)
{
  char dest[] = "TRUE";
  auto end = sal::c_str(dest, dest + sizeof(dest), true);
  EXPECT_EQ(dest + sizeof(dest), end);
  EXPECT_STREQ("true", dest);
}


TEST_F(utility, c_str_bool_true_overflow)
{
  char dest[] = "1234";
  auto end = sal::c_str(dest, dest + sizeof(dest)/2, true);
  EXPECT_EQ(dest + sizeof("true"), end);
  EXPECT_STREQ("1234", dest);
}


TEST_F(utility, c_str_bool_false)
{
  char dest[] = "................";
  auto end = sal::c_str(dest, dest + sizeof(dest), false);
  EXPECT_EQ(6U, end - dest);
  EXPECT_STREQ("false", dest);
}


TEST_F(utility, c_str_bool_false_exact)
{
  char dest[] = "FALSE";
  auto end = sal::c_str(dest, dest + sizeof(dest), false);
  EXPECT_EQ(dest + sizeof(dest), end);
  EXPECT_STREQ("false", dest);
}


TEST_F(utility, c_str_bool_false_overflow)
{
  char dest[] = "12345";
  auto end = sal::c_str(dest, dest + sizeof(dest)/2, false);
  EXPECT_EQ(dest + sizeof("false"), end);
  EXPECT_STREQ("12345", dest);
}


TEST_F(utility, c_str_nullptr)
{
  char dest[] = "................";
  auto end = sal::c_str(dest, dest + sizeof(dest), nullptr);
  EXPECT_EQ(7U, end - dest);
  EXPECT_STREQ("(null)", dest);
}


TEST_F(utility, c_str_nullptr_exact)
{
  char dest[] = "(NULL)";
  auto end = sal::c_str(dest, dest + sizeof(dest), nullptr);
  EXPECT_EQ(dest + sizeof(dest), end);
  EXPECT_STREQ("(null)", dest);
}


TEST_F(utility, c_str_nullptr_overflow)
{
  char dest[] = "123456";
  auto end = sal::c_str(dest, dest + sizeof(dest)/2, nullptr);
  EXPECT_EQ(dest + sizeof("(null)"), end);
  EXPECT_STREQ("123456", dest);
}


template <typename T>
struct c_str
  : public sal_test::with_type<T>
{
  char dest[65] = "................................................................";

  char *begin () noexcept
  {
    return dest;
  }

  char *end () noexcept
  {
    return dest + sizeof(dest);
  }

  std::string fill (T value)
  {
    auto p = sal::c_str(begin(), end(), value);
    if (p <= end())
    {
      return std::string(begin(), p);
    }
    return std::string{};
  }

  static constexpr T min () noexcept
  {
    return std::numeric_limits<T>::min();
  }

  static constexpr T zero () noexcept
  {
    return T{};
  }

  static constexpr T max () noexcept
  {
    return std::numeric_limits<T>::max();
  }

  static constexpr T between (T a, T b) noexcept
  {
    return (a + b) / 2;
  }
};
TYPED_TEST_CASE_P(c_str);


template <typename T>
std::string expected (T value)
{
  std::ostringstream oss;
  oss << value;
  return oss.str();
}


std::string expected (char value)
{
  std::ostringstream oss;
  oss << static_cast<int>(value);
  return oss.str();
}


std::string expected (unsigned char value)
{
  std::ostringstream oss;
  oss << static_cast<unsigned int>(value);
  return oss.str();
}


TYPED_TEST_P(c_str, value_min)
{
  EXPECT_EQ(
    expected(this->min()),
    this->fill(this->min())
  );
}


TYPED_TEST_P(c_str, value_zero)
{
  EXPECT_EQ(
    expected(this->zero()),
    this->fill(this->zero())
  );
}


TYPED_TEST_P(c_str, value_max)
{
  EXPECT_EQ(
    expected(this->max()),
    this->fill(this->max())
  );
}


TYPED_TEST_P(c_str, value_between_min_and_zero)
{
  TypeParam value = this->between(this->min(), this->zero());
  EXPECT_EQ(expected(value), this->fill(value));
}


TYPED_TEST_P(c_str, value_between_zero_and_max)
{
  TypeParam value = this->between(this->zero(), this->max());
  EXPECT_EQ(expected(value), this->fill(value));
}


TYPED_TEST_P(c_str, exact_room)
{
  auto value = this->max();
  auto as_string = expected(value);
  auto size = as_string.size();
  ASSERT_LT(size, sizeof(this->dest));

  // +1 for NUL
  auto last = sal::c_str(this->begin(), this->begin() + size + 1, value);
  EXPECT_EQ(this->begin() + size, last);
  EXPECT_EQ(as_string, this->begin());
  EXPECT_EQ('\0', *last);
}


TYPED_TEST_P(c_str, one_char_more_room)
{
  auto value = this->max();
  auto as_string = expected(value);
  auto size = as_string.size();
  ASSERT_LT(size + 1, sizeof(this->dest));

  // +1 for NUL
  auto last = sal::c_str(this->begin(), this->begin() + size + 2, value);
  EXPECT_EQ(this->begin() + size, last);
  EXPECT_EQ(as_string, this->begin());
  EXPECT_EQ('\0', *last);
}


TYPED_TEST_P(c_str, one_char_less_room)
{
  auto value = this->max();
  auto as_string = expected(value);
  auto size = as_string.size();
  ASSERT_LT(size, sizeof(this->dest));

  // no room for NUL
  auto last = sal::c_str(this->begin(), this->begin() + size, value);
  EXPECT_EQ(this->begin() + size, last);
  EXPECT_NE(as_string, this->begin());
}


TYPED_TEST_P(c_str, insufficient_room)
{
  auto value = this->between(this->min(), this->zero());
  auto as_string = expected(value);
  auto size = as_string.size();
  ASSERT_LT(size, sizeof(this->dest));

  // half of required room
  auto last = sal::c_str(this->begin(), this->begin() + size / 2, value);
  EXPECT_EQ(this->begin() + size, last);
  EXPECT_NE(as_string, this->begin());
}


REGISTER_TYPED_TEST_CASE_P(c_str,
  value_min,
  value_zero,
  value_max,
  value_between_min_and_zero,
  value_between_zero_and_max,
  exact_room,
  one_char_more_room,
  one_char_less_room,
  insufficient_room
);
using types = testing::Types<
  long long, unsigned long long,
  long, unsigned long,
  int, unsigned int,
  short, unsigned short,
  char, unsigned char,
  float, double, long double
>;
INSTANTIATE_TYPED_TEST_CASE_P(utility, c_str, types);


template <typename T>
struct c_str_base
  : public c_str<T>
{
  template <typename ManipValue>
  std::string fill (ManipValue value)
  {
    auto p = sal::c_str(this->begin(), this->end(), value);
    if (p <= this->end())
    {
      return std::string(this->begin(), p);
    }
    return std::string{};
  }
};
TYPED_TEST_CASE_P(c_str_base);


template <typename T, typename Manip>
std::string expected (T value, Manip manip)
{
  std::ostringstream oss;
  oss << manip << value;
  return oss.str();
}


template <typename T>
std::string expected_bin (T value)
{
  auto v = static_cast<uint64_t>(std::make_unsigned_t<T>(value));
  std::string result;
  do
  {
    result = ((v & 1) ? '1' : '0') + result;
  } while (v >>= 1);
  return result;
}


TYPED_TEST_P(c_str_base, hex_min)
{
  EXPECT_EQ(
    expected(this->min(), std::hex),
    this->fill(sal::hex(this->min()))
  );
}


TYPED_TEST_P(c_str_base, hex_zero)
{
  EXPECT_EQ(
    expected(this->zero(), std::hex),
    this->fill(sal::hex(this->zero()))
  );
}


TYPED_TEST_P(c_str_base, hex_max)
{
  EXPECT_EQ(
    expected(this->max(), std::hex),
    this->fill(sal::hex(this->max()))
  );
}


TYPED_TEST_P(c_str_base, hex_between_min_and_zero)
{
  auto value = this->between(this->min(), this->zero());
  EXPECT_EQ(expected(value, std::hex), this->fill(sal::hex(value)));
}


TYPED_TEST_P(c_str_base, hex_between_zero_and_max)
{
  auto value = this->between(this->zero(), this->max());
  EXPECT_EQ(expected(value, std::hex), this->fill(sal::hex(value)));
}


TYPED_TEST_P(c_str_base, hex_insufficient_room)
{
  auto value = this->between(this->min(), this->zero());
  auto as_string = expected(value, std::hex);
  auto size = as_string.size();
  ASSERT_LT(size, sizeof(this->dest));

  // half of required room
  auto last = sal::c_str(this->begin(), this->begin() + size / 2,
    sal::hex(value)
  );
  EXPECT_EQ(this->begin() + size, last);
  EXPECT_NE(as_string, this->begin());
}


TYPED_TEST_P(c_str_base, oct_min)
{
  EXPECT_EQ(
    expected(this->min(), std::oct),
    this->fill(sal::oct(this->min()))
  );
}


TYPED_TEST_P(c_str_base, oct_zero)
{
  EXPECT_EQ(
    expected(this->zero(), std::oct),
    this->fill(sal::oct(this->zero()))
  );
}


TYPED_TEST_P(c_str_base, oct_max)
{
  EXPECT_EQ(
    expected(this->max(), std::oct),
    this->fill(sal::oct(this->max()))
  );
}


TYPED_TEST_P(c_str_base, oct_between_min_and_zero)
{
  auto value = this->between(this->min(), this->zero());
  EXPECT_EQ(expected(value, std::oct), this->fill(sal::oct(value)));
}


TYPED_TEST_P(c_str_base, oct_between_zero_and_max)
{
  auto value = this->between(this->zero(), this->max());
  EXPECT_EQ(expected(value, std::oct), this->fill(sal::oct(value)));
}


TYPED_TEST_P(c_str_base, oct_insufficient_room)
{
  auto value = this->between(this->min(), this->zero());
  auto as_string = expected(value, std::oct);
  auto size = as_string.size();
  ASSERT_LT(size, sizeof(this->dest));

  // half of required room
  auto last = sal::c_str(this->begin(), this->begin() + size / 2,
    sal::oct(value)
  );
  EXPECT_EQ(this->begin() + size, last);
  EXPECT_NE(as_string, this->begin());
}


TYPED_TEST_P(c_str_base, bin_min)
{
  EXPECT_EQ(
    expected_bin(this->min()),
    this->fill(sal::bin(this->min()))
  );
}


TYPED_TEST_P(c_str_base, bin_zero)
{
  EXPECT_EQ(
    expected_bin(this->zero()),
    this->fill(sal::bin(this->zero()))
  );
}


TYPED_TEST_P(c_str_base, bin_max)
{
  EXPECT_EQ(
    expected_bin(this->max()),
    this->fill(sal::bin(this->max()))
  );
}


TYPED_TEST_P(c_str_base, bin_between_min_and_zero)
{
  auto value = this->between(this->min(), this->zero());
  EXPECT_EQ(expected_bin(value), this->fill(sal::bin(value)));
}


TYPED_TEST_P(c_str_base, bin_between_zero_and_max)
{
  auto value = this->between(this->zero(), this->max());
  EXPECT_EQ(expected_bin(value), this->fill(sal::bin(value)));
}


TYPED_TEST_P(c_str_base, bin_insufficient_room)
{
  auto value = this->between(this->min(), this->zero());
  auto as_string = expected_bin(value);
  auto size = as_string.size();
  ASSERT_LT(size, sizeof(this->dest));

  // half of required room
  auto last = sal::c_str(this->begin(), this->begin() + size / 2,
    sal::bin(value)
  );
  EXPECT_EQ(this->begin() + size, last);
  EXPECT_NE(as_string, this->begin());
}


REGISTER_TYPED_TEST_CASE_P(c_str_base,
  hex_min,
  hex_zero,
  hex_max,
  hex_between_min_and_zero,
  hex_between_zero_and_max,
  hex_insufficient_room,
  oct_min,
  oct_zero,
  oct_max,
  oct_between_min_and_zero,
  oct_between_zero_and_max,
  oct_insufficient_room,
  bin_min,
  bin_zero,
  bin_max,
  bin_between_min_and_zero,
  bin_between_zero_and_max,
  bin_insufficient_room
);
using base_types = testing::Types<
  long long, unsigned long long,
  long, unsigned long,
  int, unsigned int,
  short, unsigned short
>;
INSTANTIATE_TYPED_TEST_CASE_P(utility, c_str_base, base_types);


} // namespace
