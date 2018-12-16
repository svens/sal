#include <sal/format.hpp>
#include <sal/common.test.hpp>


namespace {


struct format
  : public sal_test::fixture
{
  static constexpr size_t size = 128;
  char data[size], *begin{data}, *end{data + sizeof(data)};
  sal::memory_writer_t writer{data};

  format ()
  {
    std::fill_n(data, sizeof(data), '.');
  }

  format (const format &) = delete;
  format &operator= (const format &) = delete;
};


TEST_F(format, bool_true)
{
  EXPECT_TRUE(bool(writer << true));
  EXPECT_EQ("true.", std::string(data, data + 5));
}


TEST_F(format, bool_true_exact)
{
  sal::memory_writer_t w{data, data + 4};
  EXPECT_TRUE(bool(w << true));
  EXPECT_TRUE(w.full());
  EXPECT_EQ("true.", std::string(data, data + 5));
}


TEST_F(format, bool_true_overflow)
{
  sal::memory_writer_t w{data, data + 2};
  EXPECT_FALSE(bool(w << true));
  EXPECT_EQ("..", std::string(data, data + 2));
}


TEST_F(format, bool_false)
{
  EXPECT_TRUE(bool(writer << false));
  EXPECT_EQ("false.", std::string(data, data + 6));
}


TEST_F(format, bool_false_exact)
{
  sal::memory_writer_t w{data, data + 5};
  EXPECT_TRUE(bool(w << false));
  EXPECT_TRUE(w.full());
  EXPECT_EQ("false.", std::string(data, data + 6));
}


TEST_F(format, bool_false_overflow)
{
  sal::memory_writer_t w{data, data + 2};
  EXPECT_FALSE(bool(w << false));
  EXPECT_EQ("..", std::string(data, data + 2));
}


TEST_F(format, nullptr)
{
  EXPECT_TRUE(bool(writer << nullptr));
  EXPECT_EQ("(null).", std::string(data, data + 7));
}


TEST_F(format, nullptr_exact)
{
  sal::memory_writer_t w{data, data + 6};
  EXPECT_TRUE(bool(w << nullptr));
  EXPECT_TRUE(w.full());
  EXPECT_EQ("(null).", std::string(data, data + 7));
}


TEST_F(format, nullptr_overflow)
{
  sal::memory_writer_t w{data, data + 2};
  EXPECT_FALSE(bool(w << nullptr));
  EXPECT_EQ("..", std::string(data, data + 2));
}


template <typename T>
struct format_number
  : public format
{
  static constexpr T min () noexcept
  {
    return std::numeric_limits<T>::min();
  }

  static constexpr T zero () noexcept
  {
    return {};
  }

  static constexpr T max () noexcept
  {
    return std::numeric_limits<T>::max();
  }

  static constexpr T between (T a, T b) noexcept
  {
    return (a + b) / 2;
  }

  std::string expected (T value)
  {
    std::ostringstream oss;
    oss << value;
    return oss.str();
  }

  std::string fill (T value) noexcept
  {
    if (writer << value)
    {
      return std::string(begin, writer.first);
    }
    return std::string{"FAIL"};
  }
};

using number_types = testing::Types<
  short, unsigned short,
  int, unsigned int,
  long, unsigned long,
  long long, unsigned long long,
  float, double, long double
>;

struct number_names
{
  template <typename T>
  static std::string GetName (int i)
  {
    (void)i;
    if constexpr (std::is_same_v<T, short>) return "short";
    else if constexpr (std::is_same_v<T, unsigned short>) return "unsigned_short";
    else if constexpr (std::is_same_v<T, int>) return "int";
    else if constexpr (std::is_same_v<T, unsigned int>) return "unsigned_int";
    else if constexpr (std::is_same_v<T, long>) return "long";
    else if constexpr (std::is_same_v<T, unsigned long>) return "unsigned_long";
    else if constexpr (std::is_same_v<T, long long>) return "long_long";
    else if constexpr (std::is_same_v<T, unsigned long long>) return "unsigned_long_long";
    else if constexpr (std::is_same_v<T, float>) return "float";
    else if constexpr (std::is_same_v<T, double>) return "double";
    else if constexpr (std::is_same_v<T, long double>) return "long_double";
    else return std::to_string(i);
  }
};

TYPED_TEST_CASE(format_number, number_types, number_names);


TYPED_TEST(format_number, value_min)
{
  EXPECT_EQ(
    this->expected(this->min()),
    this->fill(this->min())
  );
}


TYPED_TEST(format_number, value_zero)
{
  EXPECT_EQ(
    this->expected(this->zero()),
    this->fill(this->zero())
  );
}


TYPED_TEST(format_number, value_max)
{
  EXPECT_EQ(
    this->expected(this->max()),
    this->fill(this->max())
  );
}


TYPED_TEST(format_number, value_between_min_and_zero)
{
  TypeParam value = this->between(this->min(), this->zero());
  EXPECT_EQ(this->expected(value), this->fill(value));
}


TYPED_TEST(format_number, value_between_zero_and_max)
{
  TypeParam value = this->between(this->zero(), this->max());
  EXPECT_EQ(this->expected(value), this->fill(value));
}


TYPED_TEST(format_number, exact)
{
  auto value = this->max();
  auto as_string = this->expected(value);
  ASSERT_LT(as_string.size(), this->size);

  sal::memory_writer_t w{this->begin, this->begin + as_string.size()};
  EXPECT_TRUE(bool(w << value));
  EXPECT_TRUE(w.full());

  as_string += '.';
  EXPECT_EQ(as_string,
    std::string(this->data, this->data + as_string.size())
  );
}


TYPED_TEST(format_number, one_char_more)
{
  auto value = this->max();
  auto as_string = this->expected(value);
  ASSERT_LT(as_string.size() + 1, this->size);

  sal::memory_writer_t w{this->begin, this->begin + as_string.size() + 1};
  EXPECT_TRUE(bool(w << value));
  EXPECT_FALSE(w.full());

  as_string += '.';
  EXPECT_EQ(as_string,
    std::string(this->data, this->data + as_string.size())
  );
}


TYPED_TEST(format_number, one_char_less)
{
  auto value = this->max();
  auto as_string = this->expected(value);
  ASSERT_LT(as_string.size(), this->size);

  sal::memory_writer_t w{this->begin, this->begin + as_string.size() - 1};
  EXPECT_FALSE(bool(w << value));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ("...", std::string(this->data, this->data + 3));
}


TYPED_TEST(format_number, overflow)
{
  auto value = this->between(this->min(), this->zero());
  auto as_string = this->expected(value);
  ASSERT_LT(as_string.size(), this->size);

  sal::memory_writer_t w{this->begin, this->begin + as_string.size() / 2};
  EXPECT_FALSE(bool(w << value));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ("...", std::string(this->data, this->data + 3));
}


template <typename T>
struct format_int_base
  : public format_number<T>
{
  template <typename ManipValue>
  std::string fill (ManipValue value) noexcept
  {
    if (this->writer << value)
    {
      return std::string(this->begin, this->writer.first);
    }
    return std::string{"FAIL"};
  }
};

using int_types = testing::Types<
  long long, unsigned long long,
  long, unsigned long,
  int, unsigned int,
  short, unsigned short
>;

TYPED_TEST_CASE(format_int_base, int_types, number_names);


template <typename T, typename Manip>
std::string expected_manip (T value, Manip manip)
{
  std::ostringstream oss;
  oss << manip << value;
  return oss.str();
}


#if __apple_build_version__

// Bug in xcode8.3 iostreams: negative long formatted as octal is wrong
// fall back to printf

std::string expected_manip (long value, decltype(std::oct) manip)
{
  char buf[64];
  if (manip == std::oct)
  {
    snprintf(buf, sizeof(buf), "%lo", value);
  }
  else if (manip == std::hex)
  {
    snprintf(buf, sizeof(buf), "%lx", value);
  }
  return buf;
}

#endif


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


TYPED_TEST(format_int_base, hex_min)
{
  EXPECT_EQ(
    expected_manip(this->min(), std::hex),
    this->fill(sal::hex(this->min()))
  );
}


TYPED_TEST(format_int_base, hex_zero)
{
  EXPECT_EQ(
    expected_manip(this->zero(), std::hex),
    this->fill(sal::hex(this->zero()))
  );
}


TYPED_TEST(format_int_base, hex_max)
{
  EXPECT_EQ(
    expected_manip(this->max(), std::hex),
    this->fill(sal::hex(this->max()))
  );
}


TYPED_TEST(format_int_base, hex_between_min_and_zero)
{
  auto value = this->between(this->min(), this->zero());
  EXPECT_EQ(
    expected_manip(value, std::hex),
    this->fill(sal::hex(value))
  );
}


TYPED_TEST(format_int_base, hex_between_zero_and_max)
{
  auto value = this->between(this->zero(), this->max());
  EXPECT_EQ(
    expected_manip(value, std::hex),
    this->fill(sal::hex(value))
  );
}


TYPED_TEST(format_int_base, hex_overflow)
{
  auto value = this->between(this->min(), this->zero());
  auto as_string = expected_manip(value, std::hex);
  ASSERT_LT(as_string.size(), this->size);

  sal::memory_writer_t w{this->begin, this->begin + as_string.size() / 2};
  EXPECT_FALSE(bool(w << sal::hex(value)));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ("...", std::string(this->data, this->data + 3));
}


TYPED_TEST(format_int_base, oct_min)
{
  EXPECT_EQ(
    expected_manip(this->min(), std::oct),
    this->fill(sal::oct(this->min()))
  );
}


TYPED_TEST(format_int_base, oct_zero)
{
  EXPECT_EQ(
    expected_manip(this->zero(), std::oct),
    this->fill(sal::oct(this->zero()))
  );
}


TYPED_TEST(format_int_base, oct_max)
{
  EXPECT_EQ(
    expected_manip(this->max(), std::oct),
    this->fill(sal::oct(this->max()))
  );
}


TYPED_TEST(format_int_base, oct_between_min_and_zero)
{
  auto value = this->between(this->min(), this->zero());
  EXPECT_EQ(
    expected_manip(value, std::oct),
    this->fill(sal::oct(value))
  );
}


TYPED_TEST(format_int_base, oct_between_zero_and_max)
{
  auto value = this->between(this->zero(), this->max());
  EXPECT_EQ(
    expected_manip(value, std::oct),
    this->fill(sal::oct(value))
  );
}


TYPED_TEST(format_int_base, oct_overflow)
{
  auto value = this->between(this->min(), this->zero());
  auto as_string = expected_manip(value, std::oct);
  ASSERT_LT(as_string.size(), this->size);

  sal::memory_writer_t w{this->begin, this->begin + as_string.size() / 2};
  EXPECT_FALSE(bool(w << sal::oct(value)));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ("...", std::string(this->data, this->data + 3));
}


TYPED_TEST(format_int_base, bin_min)
{
  EXPECT_EQ(
    expected_bin(this->min()),
    this->fill(sal::bin(this->min()))
  );
}


TYPED_TEST(format_int_base, bin_zero)
{
  EXPECT_EQ(
    expected_bin(this->zero()),
    this->fill(sal::bin(this->zero()))
  );
}


TYPED_TEST(format_int_base, bin_max)
{
  EXPECT_EQ(
    expected_bin(this->max()),
    this->fill(sal::bin(this->max()))
  );
}


TYPED_TEST(format_int_base, bin_between_min_and_zero)
{
  auto value = this->between(this->min(), this->zero());
  EXPECT_EQ(
    expected_bin(value),
    this->fill(sal::bin(value))
  );
}


TYPED_TEST(format_int_base, bin_between_zero_and_max)
{
  auto value = this->between(this->zero(), this->max());
  EXPECT_EQ(
    expected_bin(value),
    this->fill(sal::bin(value))
  );
}


TYPED_TEST(format_int_base, bin_overflow)
{
  auto value = this->between(this->min(), this->zero());
  auto as_string = expected_bin(value);
  ASSERT_LT(as_string.size(), this->size);

  sal::memory_writer_t w{this->begin, this->begin + as_string.size() / 2};
  EXPECT_FALSE(bool(w << sal::bin(value)));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ("...", std::string(this->data, this->data + 3));
}


template <typename T>
std::string expected (const T *p)
{
  // every respectable compiler has own pointer formatting, let have another
  std::ostringstream oss;
  oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(p);
  return oss.str();
}


TEST_F(format, pointer)
{
  int x, *p = &x;
  EXPECT_TRUE(bool(writer << p));
  EXPECT_EQ(expected(p), std::string(begin, writer.first));
}


TEST_F(format, pointer_null)
{
  int *p = nullptr;
  EXPECT_TRUE(bool(writer << p));
  EXPECT_EQ(expected(p), std::string(begin, writer.first));
}


TEST_F(format, pointer_exact)
{
  int x, *p = &x;
  auto as_string = expected(p);
  sal::memory_writer_t w{data, data + as_string.size()};
  EXPECT_TRUE(bool(w << p));
  EXPECT_EQ(as_string, std::string(begin, w.first));
}


TEST_F(format, pointer_one_char_less)
{
  int x, *p = &x;
  auto as_string = expected(p);
  sal::memory_writer_t w{data, data + as_string.size() - 1};
  EXPECT_FALSE(bool(w << p));
  EXPECT_TRUE(w.bad());
}


TEST_F(format, pointer_one_char_more)
{
  int x, *p = &x;
  auto as_string = expected(p);
  sal::memory_writer_t w{data, data + as_string.size() + 1};
  EXPECT_TRUE(bool(w << p));
  EXPECT_FALSE(w.full());
  EXPECT_EQ(as_string, std::string(begin, w.first));
}


TEST_F(format, pointer_overflow)
{
  int x, *p = &x;
  auto as_string = expected(p);
  sal::memory_writer_t w{data, data + as_string.size() / 2};
  EXPECT_FALSE(bool(w << p));
  EXPECT_TRUE(w.bad());
}


TEST_F(format, string)
{
  EXPECT_TRUE(bool(writer << case_name));
  EXPECT_EQ(case_name, std::string(data, case_name.size()));
}


TEST_F(format, string_exact)
{
  sal::memory_writer_t w{data, data + case_name.size()};
  EXPECT_TRUE(bool(w << case_name));
  EXPECT_TRUE(w.full());
  EXPECT_EQ(case_name, std::string(data, case_name.size()));
}


TEST_F(format, string_one_char_less)
{
  sal::memory_writer_t w{data, data + case_name.size() - 1};
  EXPECT_FALSE(bool(w << case_name));
  EXPECT_TRUE(w.bad());
}


TEST_F(format, string_one_char_more)
{
  sal::memory_writer_t w{data, data + case_name.size() + 1};
  EXPECT_TRUE(bool(w << case_name));
  EXPECT_FALSE(w.full());
  EXPECT_EQ(case_name, std::string(data, case_name.size()));
}


TEST_F(format, string_overflow)
{
  sal::memory_writer_t w{data, data + case_name.size() / 2};
  EXPECT_FALSE(bool(w << case_name));
  EXPECT_TRUE(w.bad());
}


TEST_F(format, string_view)
{
  std::string_view view(case_name.data(), case_name.size());
  EXPECT_TRUE(bool(writer << view));
  EXPECT_EQ(view, std::string(data, view.size()));
}


TEST_F(format, string_view_exact)
{
  sal::memory_writer_t w{data, data + case_name.size()};
  std::string_view view(case_name.data(), case_name.size());
  EXPECT_TRUE(bool(w << view));
  EXPECT_TRUE(w.full());
  EXPECT_EQ(view, std::string(data, view.size()));
}


TEST_F(format, string_view_one_char_less)
{
  std::string_view view(case_name.data(), case_name.size());
  sal::memory_writer_t w{data, data + view.size() - 1};
  EXPECT_FALSE(bool(w << view));
  EXPECT_TRUE(w.bad());
}


TEST_F(format, string_view_one_char_more)
{
  std::string_view view(case_name.data(), case_name.size());
  sal::memory_writer_t w{data, data + view.size() + 1};
  EXPECT_TRUE(bool(w << view));
  EXPECT_FALSE(w.full());
  EXPECT_EQ(view, std::string(data, view.size()));
}


TEST_F(format, string_view_overflow)
{
  std::string_view view(case_name.data(), case_name.size());
  sal::memory_writer_t w{data, data + view.size() / 2};
  EXPECT_FALSE(bool(w << view));
  EXPECT_TRUE(w.bad());
}


template <typename T>
struct format_float
  : public format
{
  constexpr T lowest () const
  {
    return static_cast<T>(std::numeric_limits<int>::min()) + 0.123f;
  }

  constexpr T highest () const
  {
    return static_cast<T>(std::numeric_limits<int>::max()) - 0.123f;
  }

  constexpr T zero () const
  {
    return T{};
  }

  std::string expected (T value)
  {
    std::ostringstream oss;
    oss << std::setprecision(6) << std::fixed << value;
    return oss.str();
  }

  std::string fill (T value)
  {
    writer << sal::fixed_float(value, 6);
    return std::string(data, writer.first);
  }
};

using float_types = testing::Types<float, double, long double>;

TYPED_TEST_CASE(format_float, float_types, number_names);


TYPED_TEST(format_float, nan_minus)
{
  auto value = -std::numeric_limits<TypeParam>::quiet_NaN();
  EXPECT_EQ("nan", this->fill(value));
}


TYPED_TEST(format_float, nan_plus)
{
  auto value = -std::numeric_limits<TypeParam>::quiet_NaN();
  EXPECT_EQ("nan", this->fill(value));
}


TYPED_TEST(format_float, inf_minus)
{
  auto value = -std::numeric_limits<TypeParam>::infinity();
  EXPECT_EQ("-inf", this->fill(value));
}


TYPED_TEST(format_float, inf_plus)
{
  auto value = std::numeric_limits<TypeParam>::infinity();
  EXPECT_EQ("inf", this->fill(value));
}


TYPED_TEST(format_float, value_min)
{
  EXPECT_EQ(
    this->expected(this->lowest()),
    this->fill(this->lowest())
  );
}


TYPED_TEST(format_float, value_max)
{
  EXPECT_EQ(
    this->expected(this->highest()),
    this->fill(this->highest())
  );
}


TYPED_TEST(format_float, value_zero)
{
  EXPECT_EQ(
    this->expected(this->zero()),
    this->fill(this->zero())
  );
}


TYPED_TEST(format_float, value_between_lowest_and_zero)
{
  auto value = (this->lowest() + this->zero()) / 2;
  EXPECT_EQ(
    this->expected(value),
    this->fill(value)
  );
}


TYPED_TEST(format_float, value_between_zero_and_highest)
{
  auto value = (this->zero() + this->highest()) / 2;
  EXPECT_EQ(
    this->expected(value),
    this->fill(value)
  );
}


} // namespace
