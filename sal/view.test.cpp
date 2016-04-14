#include <sal/view.hpp>
#include <sal/common.test.hpp>
#include <limits>


namespace dest_is_range {


template <typename T>
std::string expected_view (const T &value)
{
  std::ostringstream oss;
  oss << std::boolalpha << value;
  return oss.str();
}


// T -> unsigned T (bool as uint8_t)
// need this to generate std::hex view using std::ostream
template <typename T>
using unsigned_t = std::make_unsigned_t<
  std::conditional_t<std::is_same<bool, T>::value, uint8_t, T>
>;


template <typename T>
std::string expected_hex_view (const T &value)
{
  std::ostringstream oss;
  oss << std::hex << static_cast<uint64_t>(static_cast<unsigned_t<T>>(value));
  return oss.str();
}


template <typename T>
std::string expected_oct_view (const T &value)
{
  std::ostringstream oss;
  oss << std::oct << static_cast<uint64_t>(static_cast<unsigned_t<T>>(value));
  return oss.str();
}


template <typename T>
std::string expected_bin_view (const T &value)
{
  auto v = static_cast<uint64_t>(static_cast<unsigned_t<T>>(value));
  std::string result;
  do
  {
    result = ((v & 1) ? '1' : '0') + result;
  } while (v >>= 1);
  return result;
}


template <typename T>
std::string expected_view (const T *value)
{
  // gcc & msvc formats differ for ptr
  // need to format as hex uintptr for uniform formatting
  std::ostringstream oss;
  oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(value);
  return oss.str();
}


template <typename T>
std::string expected_view (T *value)
{
  return expected_view(const_cast<const T *>(value));
}


std::string expected_view (const char *value)
{
  if (value)
  {
    std::ostringstream oss;
    oss << value;
    return oss.str();
  }

  return "(null)";
}


std::string expected_view (char *value)
{
  return expected_view(const_cast<const char *>(value));
}


// -- integral types --


template <typename T>
void load_min (T &value) noexcept
{
  value = std::numeric_limits<T>::min();
}


template <typename T>
void load_zero (T &value) noexcept
{
  value = static_cast<T>(0);
}


template <typename T>
void load_max (T &value) noexcept
{
  value = std::numeric_limits<T>::max();
}


template <typename T>
void load_min_zero (T &value) noexcept
{
  T min, zero;
  load_min(min);
  load_zero(zero);
  value = (min + zero)/2;
}


template <typename T>
void load_zero_max (T &value) noexcept
{
  T zero, max;
  load_zero(zero);
  load_max(max);
  value = (zero + max)/2;
}


void load_min_zero (bool &value) noexcept
{
  load_min(value);
}


void load_zero_max (bool &value) noexcept
{
  load_max(value);
}


// -- pointer types --


template <typename T>
void load_min (T *&value) noexcept
{
  value = nullptr;
}


template <typename T>
void load_zero (T *&value) noexcept
{
  load_min(value);
}


template <typename T>
void load_max (T *&value) noexcept
{
  value = reinterpret_cast<T *>(std::numeric_limits<uintptr_t>::max());
}


void load_max (const char *&value) noexcept
{
  static const char *data = "max";
  value = data;
}


void load_max (char *&value) noexcept
{
  static char data[] = "max";
  value = data;
}


void load_min_zero (const char *&value) noexcept
{
  static const char *data = "data";
  value = data;
}


void load_min_zero (char *&value) noexcept
{
  static char data[] = "data";
  value = data;
}


template <typename T>
void load_min_zero (T *&value) noexcept
{
  static int data;
  value = reinterpret_cast<T *>(&data);
}


template <typename T>
void load_zero_max (T *&value) noexcept
{
  load_min_zero(value);
}


// -- string --


void load_min (std::string &value) noexcept
{
  value = "";
}


void load_zero (std::string &value) noexcept
{
  value = "";
}


void load_max (std::string &value) noexcept
{
  value = "max";
}


void load_min_zero (std::string &value) noexcept
{
  value = "min_zero";
}


void load_zero_max (std::string &value) noexcept
{
  value = "zero_max";
}


// -- class_with_ostream --


struct class_with_ostream
{
  friend inline
  std::ostream &operator<< (std::ostream &os, const class_with_ostream &)
  {
    return (os << "class_with_ostream");
  }
};


void load_min (class_with_ostream &) noexcept
{
}


void load_zero (class_with_ostream &) noexcept
{
}


void load_max (class_with_ostream &) noexcept
{
}


void load_min_zero (class_with_ostream &) noexcept
{
}


void load_zero_max (class_with_ostream &) noexcept
{
}


template <typename T>
struct copy_v:
  public sal_test::with_type<T>
{
  static constexpr size_t view_size = 128;


  void test_hex (const T &, std::false_type /*is_integral<T>*/)
  {
    // unused
  }


  void test_hex (const T &value, std::true_type /*is_integral<T>*/)
  {
    char view[view_size];
    auto end = sal::copy_v(sal::hex(value), view, view + view_size);
    EXPECT_EQ(expected_hex_view(value), std::string(view, end));
  }


  void test_oct (const T &, std::false_type /*is_integral<T>*/)
  {
    // unused
  }


  void test_oct (const T &value, std::true_type /*is_integral<T>*/)
  {
    char view[view_size];
    auto end = sal::copy_v(sal::oct(value), view, view + view_size);
    EXPECT_EQ(expected_oct_view(value), std::string(view, end));
  }


  void test_bin (const T &, std::false_type /*is_integral<T>*/)
  {
    // unused
  }


  void test_bin (const T &value, std::true_type /*is_integral<T>*/)
  {
    char view[view_size];
    auto end = sal::copy_v(sal::bin(value), view, view + view_size);
    EXPECT_EQ(expected_bin_view(value), std::string(view, end));
  }


  void test_value (const T &value)
  {
    // test default view
    char view[view_size];
    auto end = sal::copy_v(value, view, view + view_size);
    EXPECT_GE(view + view_size, end);
    EXPECT_EQ(expected_view(value), std::string(view, end));

    // test hex/oct view (for integral types only)
    test_hex(value, std::is_integral<T>());
    test_oct(value, std::is_integral<T>());
    test_bin(value, std::is_integral<T>());
  }


  void test_buffer_zero (const T &value)
  {
    char view[view_size];
    *std::fill_n(view, view_size - 1, '.') = '\0';
    const std::string expected = view;
    EXPECT_LT(view, sal::copy_v(value, view, view));
    EXPECT_EQ(expected, view);
  }


  void test_buffer_one_less (const T &value)
  {
    char view[view_size];
    *std::fill_n(view, view_size - 1, '.') = '\0';
    const std::string expected = view;
    auto expected_end = sal::copy_v(value, view, view);
    auto expected_size = expected_end - view;
    EXPECT_EQ(expected_end,
      sal::copy_v(value, view, view + expected_size - 1)
    );
    EXPECT_EQ(expected, view);
  }


  void test_buffer_exact (const T &value)
  {
    char view[view_size];
    *std::fill_n(view, view_size - 1, '.') = '\0';
    const std::string expected = view;
    auto expected_end = sal::copy_v(value, view, view);
    auto expected_size = expected_end - view;

    // [first,last)
    {
      EXPECT_EQ(expected_end, sal::copy_v(value, view, view + expected_size));
      EXPECT_EQ(expected_view(value), std::string(view, expected_size));
    }

    // view[]
    {
      EXPECT_EQ(expected_end, sal::copy_v(value, view));
      EXPECT_EQ(expected_view(value), std::string(view, expected_size));
    }
  }
};


TYPED_TEST_CASE_P(copy_v);


TYPED_TEST_P(copy_v, value_min)
{
  TypeParam value;
  load_min(value);
  this->test_value(value);
}


TYPED_TEST_P(copy_v, value_between_min_zero)
{
  TypeParam value;
  load_min_zero(value);
  this->test_value(value);
}


TYPED_TEST_P(copy_v, value_zero)
{
  TypeParam value;
  load_zero(value);
  this->test_value(value);
}


TYPED_TEST_P(copy_v, value_between_zero_max)
{
  TypeParam value;
  load_zero_max(value);
  this->test_value(value);
}


TYPED_TEST_P(copy_v, value_max)
{
  TypeParam value;
  load_max(value);
  this->test_value(value);
}


TYPED_TEST_P(copy_v, buffer_zero)
{
  TypeParam value;
  load_zero_max(value);
  this->test_buffer_zero(value);
}


TYPED_TEST_P(copy_v, buffer_one_less)
{
  TypeParam value;
  load_zero_max(value);
  this->test_buffer_one_less(value);
}


TYPED_TEST_P(copy_v, buffer_exact)
{
  TypeParam value;
  load_zero_max(value);
  this->test_buffer_exact(value);
}


REGISTER_TYPED_TEST_CASE_P(copy_v,
  value_min,
  value_between_min_zero,
  value_zero,
  value_between_zero_max,
  value_max,
  buffer_zero,
  buffer_one_less,
  buffer_exact
);


using types = testing::Types<bool,
  char, signed char, unsigned char,
  int16_t, uint16_t,
  int32_t, uint32_t,
  int64_t, uint64_t,
  float, double, long double,
  const void *, void *,
  const char *, char *,
  std::string,
  class_with_ostream
>;


INSTANTIATE_TYPED_TEST_CASE_P(view, copy_v, types);


} // namespace dest_is_range


namespace source_is_const_dest_is_range {


using copy_v = sal_test::with_value<bool>;


TEST_P(copy_v, empty_array_to_range)
{
  char source[] = "", dest[128];
  auto end = sal::copy_v(source, dest, dest + sizeof(dest));
  EXPECT_EQ(dest, end);
}


TEST_P(copy_v, array_to_bigger_range)
{
  char source[] = "12", dest[] = "abc";
  auto end = sal::copy_v(source, dest, dest + sizeof(dest) - 1);
  ASSERT_EQ(dest + sizeof(source) - 1, end);
  *end = '\0';
  EXPECT_STREQ("12", dest);
}


TEST_P(copy_v, array_to_equal_range)
{
  char source[] = "123", dest[] = "abc";
  auto end = sal::copy_v(source, dest, dest + sizeof(dest) - 1);
  ASSERT_EQ(dest + sizeof(source) - 1, end);
  EXPECT_STREQ("123", dest);
}


TEST_P(copy_v, array_to_smaller_range)
{
  char source[] = "123", dest[] = "ab";
  auto end = sal::copy_v(source, dest, dest + sizeof(dest) - 1);
  ASSERT_EQ(dest + sizeof(source) - 1, end);
  EXPECT_STREQ("ab", dest);
}


TEST_P(copy_v, empty_array_to_array)
{
  char source[] = "", dest[128];
  auto end = sal::copy_v(source, dest);
  EXPECT_EQ(dest, end);
}


TEST_P(copy_v, array_to_bigger_array)
{
  char source[] = "12", dest[] = "abc";
  auto end = sal::copy_v(source, dest);
  ASSERT_EQ(dest + sizeof(source) - 1, end);
  *end = '\0';
  EXPECT_STREQ("12", dest);
}


TEST_P(copy_v, array_to_equal_array)
{
  char source[] = "123", dest[] = "abc";
  auto end = sal::copy_v(source, dest);
  ASSERT_EQ(dest + sizeof(source) - 1, end);
  EXPECT_STREQ("123", dest);
}


TEST_P(copy_v, array_to_smaller_array)
{
  // dest final NUL will be overwritten by '3'
  char source[] = "123", dest[] = "ab";
  auto end = sal::copy_v(source, dest);
  ASSERT_EQ(dest + sizeof(dest), end);
  EXPECT_EQ("123", std::string(dest, end));

  // making dest even smaller would trigger static_assert() failure
}


INSTANTIATE_TEST_CASE_P(view, copy_v, testing::Values(true));


} // namespace source_and_dest_are_const
