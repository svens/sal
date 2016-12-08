#include <sal/memory_writer.hpp>
#include <sal/common.test.hpp>


namespace {


struct memory_writer
  : public sal_test::fixture
{
  static constexpr size_t size = 128;
  char data[size], *begin{data}, *end{data + sizeof(data)};
  sal::memory_writer_t writer{data};

  memory_writer ()
  {
    std::fill_n(data, sizeof(data), '.');
  }
};
constexpr size_t memory_writer::size;


TEST_F(memory_writer, ctor_range)
{
  sal::memory_writer_t a{begin, end};

  EXPECT_TRUE(bool(a));
  EXPECT_TRUE(a.good());
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_EQ(size, a.size());
  EXPECT_EQ(begin, a.begin());
  EXPECT_EQ(end, a.end());
}


TEST_F(memory_writer, ctor_array)
{
  sal::memory_writer_t a{data};

  EXPECT_TRUE(bool(a));
  EXPECT_TRUE(a.good());
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_EQ(size, a.size());
  EXPECT_EQ(begin, a.begin());
  EXPECT_EQ(end, a.end());
}


TEST_F(memory_writer, ctor_empty_range)
{
  sal::memory_writer_t a{begin, begin};

  EXPECT_TRUE(bool(a));
  EXPECT_TRUE(a.good());
  EXPECT_TRUE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_EQ(0U, a.size());
  EXPECT_EQ(begin, a.begin());
  EXPECT_EQ(begin, a.end());
}


TEST_F(memory_writer, ctor_invalid_range)
{
  sal::memory_writer_t a{end, begin};
  EXPECT_FALSE(bool(a));
  EXPECT_FALSE(a.good());
  EXPECT_FALSE(a.full());
  EXPECT_TRUE(a.bad());
}


TEST_F(memory_writer, ctor_move)
{
  uint32_t d[32];
  sal::memory_writer_t a{d};
  EXPECT_TRUE(bool(a));
  EXPECT_TRUE(a.good());
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());

  auto b{std::move(a)};
  EXPECT_TRUE(bool(b));
  EXPECT_TRUE(b.good());
  EXPECT_FALSE(b.full());
  EXPECT_FALSE(b.bad());
  EXPECT_EQ(sizeof(d), b.size());

  auto expected = reinterpret_cast<char *>(&d[0]);
  EXPECT_EQ(expected, b.begin());
  EXPECT_EQ(expected + sizeof(d), b.end());
}


TEST_F(memory_writer, assign_move)
{
  sal::memory_writer_t a{data};
  EXPECT_TRUE(bool(a));
  EXPECT_EQ(begin, a.begin());
  EXPECT_EQ(end, a.end());

  uint32_t d[1];
  sal::memory_writer_t b{d};
  EXPECT_TRUE(bool(b));
  auto expected = reinterpret_cast<char *>(&d[0]);
  EXPECT_EQ(expected, b.begin());
  EXPECT_EQ(expected + sizeof(d), b.end());

  b = std::move(a);
  EXPECT_TRUE(bool(b));
  EXPECT_EQ(begin, b.begin());
  EXPECT_EQ(end, b.end());
}


TEST_F(memory_writer, swap)
{
  sal::memory_writer_t a{data};
  EXPECT_TRUE(bool(a));
  EXPECT_EQ(begin, a.begin());
  EXPECT_EQ(end, a.end());

  uint32_t d[1];
  sal::memory_writer_t b{d};
  EXPECT_TRUE(bool(b));
  auto expected = reinterpret_cast<char *>(&d[0]);
  EXPECT_EQ(expected, b.begin());
  EXPECT_EQ(expected + sizeof(d), b.end());

  a.swap(b);

  EXPECT_TRUE(bool(b));
  EXPECT_EQ(begin, b.begin());
  EXPECT_EQ(end, b.end());

  EXPECT_TRUE(bool(a));
  EXPECT_EQ(expected, a.begin());
  EXPECT_EQ(expected + sizeof(d), a.end());
}


TEST_F(memory_writer, print)
{
  writer.print("hello", ',', ' ', "world");
  EXPECT_EQ("hello, world", std::string(data, writer.first));
}


template <typename T>
using write = sal_test::with_type<T>;
TYPED_TEST_CASE_P(write);


TYPED_TEST_P(write, single)
{
  TypeParam d{};
  sal::memory_writer_t a{&d, &d + 1};

  TypeParam expected = d + 1;
  EXPECT_TRUE(bool(a.write(expected)));
  EXPECT_TRUE(a.full());
  EXPECT_FALSE(a.bad());
  EXPECT_EQ(expected, d);

  d = 0;
  expected = d + 1;
  EXPECT_FALSE(bool(a.write(expected)));
  EXPECT_FALSE(a.full());
  EXPECT_TRUE(a.bad());
  EXPECT_NE(expected, d);
}


TYPED_TEST_P(write, range)
{
  // need [3] to test but allocate [4] to silence g++ error:
  // 'array subscript is above array bounds'
  TypeParam d[4] = { {}, {}, {} };
  sal::memory_writer_t a{d, d + 3};

  TypeParam expected[] = { {}, {} };
  expected[0]++, expected[1]++, expected[1]++;
  EXPECT_TRUE(bool(a.write(expected, expected + 2)));
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());
  EXPECT_EQ(expected[0], d[0]);
  EXPECT_EQ(expected[1], d[1]);

  EXPECT_FALSE(bool(a.write(expected, expected + 2)));
  EXPECT_FALSE(a.full());
  EXPECT_TRUE(a.bad());
  EXPECT_EQ(TypeParam{}, d[2]);
}


TYPED_TEST_P(write, array)
{
  // need [3] to test but allocate [4] to silence g++ error:
  // 'array subscript is above array bounds'
  TypeParam d[4] = { {}, {}, {} };
  sal::memory_writer_t a{d, d + 3};

  TypeParam expected[] = { {}, {} };
  expected[0]++, expected[1]++, expected[1]++;
  EXPECT_TRUE(bool(a.write(expected, expected + 2)));
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());
  EXPECT_EQ(expected[0], d[0]);
  EXPECT_EQ(expected[1], d[1]);

  EXPECT_FALSE(bool(a.write(expected)));
  EXPECT_FALSE(a.full());
  EXPECT_TRUE(a.bad());
  EXPECT_EQ(TypeParam{}, d[2]);
}


template <typename T>
using skip = sal_test::with_type<T>;
TYPED_TEST_CASE_P(skip);


TYPED_TEST_P(skip, basic)
{
  TypeParam d[] = { {}, {} };
  sal::memory_writer_t a{d};

  EXPECT_TRUE(bool(a));
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_TRUE(bool(a.skip(sizeof(TypeParam))));
  EXPECT_EQ(TypeParam{}, d[0]);

  TypeParam expected = d[1] + 1;
  EXPECT_TRUE(bool(a.write(expected)));
  EXPECT_TRUE(a.full());
  EXPECT_FALSE(a.bad());
  EXPECT_EQ(expected, d[1]);
  EXPECT_EQ(TypeParam{}, d[0]);
}


TYPED_TEST_P(skip, to_end)
{
  TypeParam d[] = { {}, {} };
  sal::memory_writer_t a{d};

  EXPECT_TRUE(bool(a));
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_TRUE(bool(a.skip(2*sizeof(TypeParam))));
  EXPECT_TRUE(a.full());
  EXPECT_FALSE(a.bad());
  EXPECT_EQ(TypeParam{}, d[0]);
  EXPECT_EQ(TypeParam{}, d[1]);
}


TYPED_TEST_P(skip, past_end)
{
  TypeParam d[] = { {}, {} };
  sal::memory_writer_t a{d};

  EXPECT_TRUE(bool(a));
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_FALSE(bool(a.skip(3*sizeof(TypeParam))));
  EXPECT_FALSE(a.full());
  EXPECT_TRUE(a.bad());
  EXPECT_EQ(TypeParam{}, d[0]);
  EXPECT_EQ(TypeParam{}, d[1]);
}


TEST_F(memory_writer, inserter_bool_true)
{
  EXPECT_TRUE(bool(writer << true));
  EXPECT_EQ("true.", std::string(data, data + 5));
}


TEST_F(memory_writer, inserter_bool_true_exact)
{
  sal::memory_writer_t w{data, data + 4};
  EXPECT_TRUE(bool(w << true));
  EXPECT_TRUE(w.full());
  EXPECT_EQ("true.", std::string(data, data + 5));
}


TEST_F(memory_writer, inserter_bool_true_overflow)
{
  sal::memory_writer_t w{data, data + 2};
  EXPECT_FALSE(bool(w << true));
  EXPECT_EQ("..", std::string(data, data + 2));
}


TEST_F(memory_writer, inserter_bool_false)
{
  EXPECT_TRUE(bool(writer << false));
  EXPECT_EQ("false.", std::string(data, data + 6));
}


TEST_F(memory_writer, inserter_bool_false_exact)
{
  sal::memory_writer_t w{data, data + 5};
  EXPECT_TRUE(bool(w << false));
  EXPECT_TRUE(w.full());
  EXPECT_EQ("false.", std::string(data, data + 6));
}


TEST_F(memory_writer, inserter_bool_false_overflow)
{
  sal::memory_writer_t w{data, data + 2};
  EXPECT_FALSE(bool(w << false));
  EXPECT_EQ("..", std::string(data, data + 2));
}


TEST_F(memory_writer, inserter_nullptr)
{
  EXPECT_TRUE(bool(writer << nullptr));
  EXPECT_EQ("(null).", std::string(data, data + 7));
}


TEST_F(memory_writer, inserter_nullptr_exact)
{
  sal::memory_writer_t w{data, data + 6};
  EXPECT_TRUE(bool(w << nullptr));
  EXPECT_TRUE(w.full());
  EXPECT_EQ("(null).", std::string(data, data + 7));
}


TEST_F(memory_writer, inserter_nullptr_overflow)
{
  sal::memory_writer_t w{data, data + 2};
  EXPECT_FALSE(bool(w << nullptr));
  EXPECT_EQ("..", std::string(data, data + 2));
}


template <typename T>
using char_inserter = sal_test::with_type<T>;
TYPED_TEST_CASE_P(char_inserter);


TYPED_TEST_P(char_inserter, basic)
{
  char d[2];
  TypeParam expected = 'a';
  sal::memory_writer_t w{d};
  EXPECT_TRUE(bool(w << expected));
  EXPECT_EQ(expected, d[0]);
}


TYPED_TEST_P(char_inserter, exact)
{
  char d{};
  TypeParam expected = 'a';
  sal::memory_writer_t w{&d, &d + sizeof(d)};
  EXPECT_TRUE(bool(w << expected));
  EXPECT_TRUE(w.full());
  EXPECT_EQ(expected, d);
}


TYPED_TEST_P(char_inserter, overflow)
{
  char d{};
  TypeParam expected = 'a';
  sal::memory_writer_t w{&d, &d};
  EXPECT_FALSE(bool(w << expected));
  EXPECT_EQ('\0', d);
}


TEST_F(memory_writer, c_str)
{
  char expected[] = "123";
  EXPECT_TRUE(bool(writer << expected));
  EXPECT_EQ("123.", std::string(data, data + 4));
}


TEST_F(memory_writer, c_str_exact)
{
  char expected[] = "123";
  sal::memory_writer_t w{data, data + sizeof(expected) - 1};
  EXPECT_TRUE(bool(w << expected));
  EXPECT_TRUE(w.full());
  EXPECT_EQ("123.", std::string(data, data + 4));
}


TEST_F(memory_writer, c_str_one_char_more)
{
  char expected[] = "123";
  sal::memory_writer_t w{data, data + sizeof(expected)};
  EXPECT_TRUE(bool(w << expected));
  EXPECT_FALSE(w.full());
  EXPECT_EQ("123.", std::string(data, data + 4));
}


TEST_F(memory_writer, c_str_one_char_less)
{
  char expected[] = "123";
  sal::memory_writer_t w{data, data + sizeof(expected) - 2};
  EXPECT_FALSE(bool(w << expected));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ("12..", std::string(data, data + sizeof(expected)));
}


TEST_F(memory_writer, c_str_overflow)
{
  char expected[] = "12345";
  sal::memory_writer_t w{data, data + sizeof(expected) / 2};
  EXPECT_FALSE(bool(w << expected));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ("123...", std::string(data, data + sizeof(expected)));
}


template <typename T>
struct int_inserter
  : public memory_writer
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
TYPED_TEST_CASE_P(int_inserter);


TYPED_TEST_P(int_inserter, value_min)
{
  EXPECT_EQ(
    this->expected(this->min()),
    this->fill(this->min())
  );
}


TYPED_TEST_P(int_inserter, value_zero)
{
  EXPECT_EQ(
    this->expected(this->zero()),
    this->fill(this->zero())
  );
}


TYPED_TEST_P(int_inserter, value_max)
{
  EXPECT_EQ(
    this->expected(this->max()),
    this->fill(this->max())
  );
}


TYPED_TEST_P(int_inserter, value_between_min_and_zero)
{
  TypeParam value = this->between(this->min(), this->zero());
  EXPECT_EQ(this->expected(value), this->fill(value));
}


TYPED_TEST_P(int_inserter, value_between_zero_and_max)
{
  TypeParam value = this->between(this->zero(), this->max());
  EXPECT_EQ(this->expected(value), this->fill(value));
}


TYPED_TEST_P(int_inserter, exact)
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


TYPED_TEST_P(int_inserter, one_char_more)
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


TYPED_TEST_P(int_inserter, one_char_less)
{
  auto value = this->max();
  auto as_string = this->expected(value);
  ASSERT_LT(as_string.size(), this->size);

  sal::memory_writer_t w{this->begin, this->begin + as_string.size() - 1};
  EXPECT_FALSE(bool(w << value));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ("...", std::string(this->data, this->data + 3));
}


TYPED_TEST_P(int_inserter, overflow)
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
struct base_inserter
  : public int_inserter<T>
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
TYPED_TEST_CASE_P(base_inserter);


template <typename T, typename Manip>
std::string expected_manip (T value, Manip manip)
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


TYPED_TEST_P(base_inserter, hex_min)
{
  EXPECT_EQ(
    expected_manip(this->min(), std::hex),
    this->fill(sal::hex(this->min()))
  );
}


TYPED_TEST_P(base_inserter, hex_zero)
{
  EXPECT_EQ(
    expected_manip(this->zero(), std::hex),
    this->fill(sal::hex(this->zero()))
  );
}


TYPED_TEST_P(base_inserter, hex_max)
{
  EXPECT_EQ(
    expected_manip(this->max(), std::hex),
    this->fill(sal::hex(this->max()))
  );
}


TYPED_TEST_P(base_inserter, hex_between_min_and_zero)
{
  auto value = this->between(this->min(), this->zero());
  EXPECT_EQ(
    expected_manip(value, std::hex),
    this->fill(sal::hex(value))
  );
}


TYPED_TEST_P(base_inserter, hex_between_zero_and_max)
{
  auto value = this->between(this->zero(), this->max());
  EXPECT_EQ(
    expected_manip(value, std::hex),
    this->fill(sal::hex(value))
  );
}


TYPED_TEST_P(base_inserter, hex_overflow)
{
  auto value = this->between(this->min(), this->zero());
  auto as_string = expected_manip(value, std::hex);
  ASSERT_LT(as_string.size(), this->size);

  sal::memory_writer_t w{this->begin, this->begin + as_string.size() / 2};
  EXPECT_FALSE(bool(w << sal::hex(value)));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ("...", std::string(this->data, this->data + 3));
}


TYPED_TEST_P(base_inserter, oct_min)
{
  EXPECT_EQ(
    expected_manip(this->min(), std::oct),
    this->fill(sal::oct(this->min()))
  );
}


TYPED_TEST_P(base_inserter, oct_zero)
{
  EXPECT_EQ(
    expected_manip(this->zero(), std::oct),
    this->fill(sal::oct(this->zero()))
  );
}


TYPED_TEST_P(base_inserter, oct_max)
{
  EXPECT_EQ(
    expected_manip(this->max(), std::oct),
    this->fill(sal::oct(this->max()))
  );
}


TYPED_TEST_P(base_inserter, oct_between_min_and_zero)
{
  auto value = this->between(this->min(), this->zero());
  EXPECT_EQ(
    expected_manip(value, std::oct),
    this->fill(sal::oct(value))
  );
}


TYPED_TEST_P(base_inserter, oct_between_zero_and_max)
{
  auto value = this->between(this->zero(), this->max());
  EXPECT_EQ(
    expected_manip(value, std::oct),
    this->fill(sal::oct(value))
  );
}


TYPED_TEST_P(base_inserter, oct_overflow)
{
  auto value = this->between(this->min(), this->zero());
  auto as_string = expected_manip(value, std::oct);
  ASSERT_LT(as_string.size(), this->size);

  sal::memory_writer_t w{this->begin, this->begin + as_string.size() / 2};
  EXPECT_FALSE(bool(w << sal::oct(value)));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ("...", std::string(this->data, this->data + 3));
}


TYPED_TEST_P(base_inserter, bin_min)
{
  EXPECT_EQ(
    expected_bin(this->min()),
    this->fill(sal::bin(this->min()))
  );
}


TYPED_TEST_P(base_inserter, bin_zero)
{
  EXPECT_EQ(
    expected_bin(this->zero()),
    this->fill(sal::bin(this->zero()))
  );
}


TYPED_TEST_P(base_inserter, bin_max)
{
  EXPECT_EQ(
    expected_bin(this->max()),
    this->fill(sal::bin(this->max()))
  );
}


TYPED_TEST_P(base_inserter, bin_between_min_and_zero)
{
  auto value = this->between(this->min(), this->zero());
  EXPECT_EQ(
    expected_bin(value),
    this->fill(sal::bin(value))
  );
}


TYPED_TEST_P(base_inserter, bin_between_zero_and_max)
{
  auto value = this->between(this->zero(), this->max());
  EXPECT_EQ(
    expected_bin(value),
    this->fill(sal::bin(value))
  );
}


TYPED_TEST_P(base_inserter, bin_overflow)
{
  auto value = this->between(this->min(), this->zero());
  auto as_string = expected_bin(value);
  ASSERT_LT(as_string.size(), this->size);

  sal::memory_writer_t w{this->begin, this->begin + as_string.size() / 2};
  EXPECT_FALSE(bool(w << sal::bin(value)));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ("...", std::string(this->data, this->data + 3));
}


REGISTER_TYPED_TEST_CASE_P(write,
  single, range, array
);
REGISTER_TYPED_TEST_CASE_P(skip,
  basic, to_end, past_end
);
REGISTER_TYPED_TEST_CASE_P(char_inserter,
  basic, exact, overflow
);
REGISTER_TYPED_TEST_CASE_P(int_inserter,
  value_min,
  value_zero,
  value_max,
  value_between_min_and_zero,
  value_between_zero_and_max,
  exact,
  one_char_more,
  one_char_less,
  overflow
);
REGISTER_TYPED_TEST_CASE_P(base_inserter,
  hex_min,
  hex_zero,
  hex_max,
  hex_between_min_and_zero,
  hex_between_zero_and_max,
  hex_overflow,
  oct_min,
  oct_zero,
  oct_max,
  oct_between_min_and_zero,
  oct_between_zero_and_max,
  oct_overflow,
  bin_min,
  bin_zero,
  bin_max,
  bin_between_min_and_zero,
  bin_between_zero_and_max,
  bin_overflow
);

using types = testing::Types<uint8_t, uint16_t, uint32_t, uint64_t, float>;
INSTANTIATE_TYPED_TEST_CASE_P(memory_writer, write, types);
INSTANTIATE_TYPED_TEST_CASE_P(memory_writer, skip, types);

using char_types = testing::Types<char, signed char, unsigned char>;
INSTANTIATE_TYPED_TEST_CASE_P(memory_writer, char_inserter, char_types);

using int_types = testing::Types<
  short, unsigned short,
  int, unsigned int,
  long, unsigned long,
  long long, unsigned long long,
  float, double, long double
>;
INSTANTIATE_TYPED_TEST_CASE_P(memory_writer, int_inserter, int_types);

using base_int_types = testing::Types<
  long long, unsigned long long,
  long, unsigned long,
  int, unsigned int,
  short, unsigned short
>;
INSTANTIATE_TYPED_TEST_CASE_P(memory_writer, base_inserter, base_int_types);


template <typename T>
std::string expected (const T *p)
{
  // every respectable compiler has own pointer formatting, let have another
  std::ostringstream oss;
  oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(p);
  return oss.str();
}


TEST_F(memory_writer, pointer)
{
  int x, *p = &x;
  EXPECT_TRUE(bool(writer << p));
  EXPECT_EQ(expected(p), std::string(begin, writer.first));
}


TEST_F(memory_writer, pointer_null)
{
  int *p = nullptr;
  EXPECT_TRUE(bool(writer << p));
  EXPECT_EQ(expected(p), std::string(begin, writer.first));
}


TEST_F(memory_writer, pointer_exact)
{
  int x, *p = &x;
  auto as_string = expected(p);
  sal::memory_writer_t w{data, data + as_string.size()};
  EXPECT_TRUE(bool(w << p));
  EXPECT_EQ(as_string, std::string(begin, w.first));
}


TEST_F(memory_writer, pointer_one_char_less)
{
  int x, *p = &x;
  auto as_string = expected(p);
  sal::memory_writer_t w{data, data + as_string.size() - 1};
  EXPECT_FALSE(bool(w << p));
  EXPECT_TRUE(w.bad());
}


TEST_F(memory_writer, pointer_one_char_more)
{
  int x, *p = &x;
  auto as_string = expected(p);
  sal::memory_writer_t w{data, data + as_string.size() + 1};
  EXPECT_TRUE(bool(w << p));
  EXPECT_EQ(as_string, std::string(begin, w.first));
}


TEST_F(memory_writer, pointer_overflow)
{
  int x, *p = &x;
  auto as_string = expected(p);
  sal::memory_writer_t w{data, data + as_string.size() / 2};
  EXPECT_FALSE(bool(w << p));
  EXPECT_TRUE(w.bad());
}


} // namespace
