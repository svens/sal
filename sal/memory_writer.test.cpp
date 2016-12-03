#include <sal/memory_writer.hpp>
#include <sal/common.test.hpp>


namespace {


template <typename T>
using member = sal_test::with_type<T>;
TYPED_TEST_CASE_P(member);


TYPED_TEST_P(member, ctor_array)
{
  TypeParam d[32];
  sal::memory_writer_t a{d};

  EXPECT_TRUE(bool(a));
  EXPECT_FALSE(a.is_full());
  EXPECT_FALSE(a.is_bad());

  EXPECT_EQ(sizeof(d), a.size());

  auto expected = reinterpret_cast<char *>(&d[0]);
  EXPECT_EQ(expected, a.begin());
  EXPECT_EQ(expected + sizeof(d), a.end());
}


TYPED_TEST_P(member, ctor_range)
{
  TypeParam d[32];
  sal::memory_writer_t a{d, d + 32};

  EXPECT_TRUE(bool(a));
  EXPECT_FALSE(a.is_full());
  EXPECT_FALSE(a.is_bad());

  EXPECT_EQ(sizeof(d), a.size());

  auto expected = reinterpret_cast<char *>(&d[0]);
  EXPECT_EQ(expected, a.begin());
  EXPECT_EQ(expected + sizeof(d), a.end());
}


TYPED_TEST_P(member, ctor_empty_range)
{
  TypeParam d[32];
  sal::memory_writer_t a{d, d};
  EXPECT_TRUE(bool(a));
  EXPECT_TRUE(a.is_full());
  EXPECT_FALSE(a.is_bad());
}


TYPED_TEST_P(member, ctor_invalid_range)
{
  TypeParam d[32];
  sal::memory_writer_t a{d + 32, d};
  EXPECT_FALSE(bool(a));
  EXPECT_FALSE(a.is_full());
  EXPECT_TRUE(a.is_bad());
}


TYPED_TEST_P(member, ctor_move)
{
  TypeParam d[32];
  sal::memory_writer_t a{d};
  EXPECT_TRUE(bool(a));
  EXPECT_FALSE(a.is_full());
  EXPECT_FALSE(a.is_bad());

  auto b{std::move(a)};
  EXPECT_TRUE(bool(b));
  EXPECT_FALSE(b.is_full());
  EXPECT_FALSE(b.is_bad());
  EXPECT_EQ(sizeof(d), b.size());

  auto expected = reinterpret_cast<char *>(&d[0]);
  EXPECT_EQ(expected, b.begin());
  EXPECT_EQ(expected + sizeof(d), b.end());

  EXPECT_FALSE(bool(a));
  EXPECT_FALSE(a.is_full());
  EXPECT_TRUE(a.is_bad());
}


TYPED_TEST_P(member, write)
{
  TypeParam d{};
  sal::memory_writer_t a{&d, &d + 1};

  TypeParam expected = d + 1;
  EXPECT_TRUE(bool(a.write(expected)));
  EXPECT_TRUE(a.is_full());
  EXPECT_FALSE(a.is_bad());
  EXPECT_EQ(expected, d);

  d = 0;
  expected = d + 1;
  EXPECT_FALSE(bool(a.write(expected)));
  EXPECT_FALSE(a.is_full());
  EXPECT_TRUE(a.is_bad());
  EXPECT_NE(expected, d);
}


TYPED_TEST_P(member, write_range)
{
  // need [3] to test but allocate [4] to silence g++ error:
  // 'array subscript is above array bounds'
  TypeParam d[4] = { {}, {}, {} };
  sal::memory_writer_t a{d, d + 3};

  TypeParam expected[] = { {}, {} };
  expected[0]++, expected[1]++, expected[1]++;
  EXPECT_TRUE(bool(a.write(expected, expected + 2)));
  EXPECT_FALSE(a.is_full());
  EXPECT_FALSE(a.is_bad());
  EXPECT_EQ(expected[0], d[0]);
  EXPECT_EQ(expected[1], d[1]);

  EXPECT_FALSE(bool(a.write(expected, expected + 2)));
  EXPECT_FALSE(a.is_full());
  EXPECT_TRUE(a.is_bad());
  EXPECT_EQ(TypeParam{}, d[2]);
}


TYPED_TEST_P(member, write_array)
{
  // need [3] to test but allocate [4] to silence g++ error:
  // 'array subscript is above array bounds'
  TypeParam d[4] = { {}, {}, {} };
  sal::memory_writer_t a{d, d + 3};

  TypeParam expected[] = { {}, {} };
  expected[0]++, expected[1]++, expected[1]++;
  EXPECT_TRUE(bool(a.write(expected, expected + 2)));
  EXPECT_FALSE(a.is_full());
  EXPECT_FALSE(a.is_bad());
  EXPECT_EQ(expected[0], d[0]);
  EXPECT_EQ(expected[1], d[1]);

  EXPECT_FALSE(bool(a.write(expected)));
  EXPECT_FALSE(a.is_full());
  EXPECT_TRUE(a.is_bad());
  EXPECT_EQ(TypeParam{}, d[2]);
}


TYPED_TEST_P(member, skip)
{
  TypeParam d[] = { {}, {} };
  sal::memory_writer_t a{d};

  EXPECT_TRUE(bool(a));
  EXPECT_FALSE(a.is_full());
  EXPECT_FALSE(a.is_bad());

  EXPECT_TRUE(bool(a.skip(sizeof(TypeParam))));
  EXPECT_EQ(TypeParam{}, d[0]);

  TypeParam expected = d[1] + 1;
  EXPECT_TRUE(bool(a.write(expected)));
  EXPECT_TRUE(a.is_full());
  EXPECT_FALSE(a.is_bad());
  EXPECT_EQ(expected, d[1]);
  EXPECT_EQ(TypeParam{}, d[0]);
}


TYPED_TEST_P(member, skip_to_end)
{
  TypeParam d[] = { {}, {} };
  sal::memory_writer_t a{d};

  EXPECT_TRUE(bool(a));
  EXPECT_FALSE(a.is_full());
  EXPECT_FALSE(a.is_bad());

  EXPECT_TRUE(bool(a.skip(2*sizeof(TypeParam))));
  EXPECT_TRUE(a.is_full());
  EXPECT_FALSE(a.is_bad());
  EXPECT_EQ(TypeParam{}, d[0]);
  EXPECT_EQ(TypeParam{}, d[1]);
}


TYPED_TEST_P(member, skip_past_end)
{
  TypeParam d[] = { {}, {} };
  sal::memory_writer_t a{d};

  EXPECT_TRUE(bool(a));
  EXPECT_FALSE(a.is_full());
  EXPECT_FALSE(a.is_bad());

  EXPECT_FALSE(bool(a.skip(3*sizeof(TypeParam))));
  EXPECT_FALSE(a.is_full());
  EXPECT_TRUE(a.is_bad());
  EXPECT_EQ(TypeParam{}, d[0]);
  EXPECT_EQ(TypeParam{}, d[1]);
}


template <typename T>
struct inserter
  : public sal_test::with_type<T>
{
  char view[65] = "................................................................";
  sal::memory_writer_t writer{view};

  static T min ()
  {
    return std::numeric_limits<T>::min();
  }

  static T zero ()
  {
    return T{};
  }

  static T max ()
  {
    return std::numeric_limits<T>::max();
  }

  std::string format (T v)
  {
    writer << v;
    size_t size = writer.begin() - &view[0];
    return std::string{&view[0], size};
  }

  static std::string expected (T v)
  {
    std::ostringstream oss;
    oss << std::boolalpha << v;
    return oss.str();
  }
};
TYPED_TEST_CASE_P(inserter);


template <typename T>
inline T between (T a, T b)
{
  return (a + b) / 2;
}


template <>
inline bool between<bool> (bool a, bool b)
{
  return (a || b) ? true : false;
}


TYPED_TEST_P(inserter, value_min)
{
  EXPECT_EQ(
    this->expected(this->min()),
    this->format(this->min())
  );
}


TYPED_TEST_P(inserter, value_between_min_zero)
{
  auto v = between(this->min(), this->zero());
  EXPECT_EQ(this->expected(v), this->format(v));
}


TYPED_TEST_P(inserter, value_zero)
{
  EXPECT_EQ(
    this->expected(this->zero()),
    this->format(this->zero())
  );
}


TYPED_TEST_P(inserter, value_between_zero_max)
{
  auto v = between(this->zero(), this->max());
  EXPECT_EQ(this->expected(v), this->format(v));
}


TYPED_TEST_P(inserter, value_max)
{
  EXPECT_EQ(
    this->expected(this->max()),
    this->format(this->max())
  );
}


TYPED_TEST_P(inserter, range_empty)
{
  char v[] = "...";
  sal::memory_writer_t w{v, v};

  EXPECT_TRUE(bool(w));
  EXPECT_TRUE(w.is_full());
  EXPECT_FALSE(w.is_bad());

  std::string e = v;
  w << this->max();
  EXPECT_FALSE(bool(w));
  EXPECT_FALSE(w.is_full());
  EXPECT_TRUE(w.is_bad());
  EXPECT_EQ(e, v);
}


TYPED_TEST_P(inserter, range_one_less)
{
  char v[] = "................................";
  sal::memory_writer_t w{v};

  // first step: find length
  w << this->max();
  auto s = w.begin() - &v[0];
  EXPECT_TRUE(bool(w));
  EXPECT_FALSE(w.is_full());
  EXPECT_FALSE(w.is_bad());

  // reinitialize writer, this time with truncated range
  w = sal::memory_writer_t{v, v + s - 1};
  w << this->max();
  EXPECT_FALSE(bool(w));
  EXPECT_FALSE(w.is_full());
  EXPECT_TRUE(w.is_bad());
}


TYPED_TEST_P(inserter, range_exact)
{
  char v[] = "................................";
  sal::memory_writer_t w{v};

  // first step: find length
  w << this->max();
  auto s = w.begin() - &v[0];
  EXPECT_TRUE(bool(w));
  EXPECT_FALSE(w.is_full());
  EXPECT_FALSE(w.is_bad());

  // reinitialize writer, this time with exact range
  w = sal::memory_writer_t{v, v + s};
  w << this->max();
  EXPECT_TRUE(bool(w));
  EXPECT_TRUE(w.is_full());
  EXPECT_FALSE(w.is_bad());
}


template <typename T>
struct inserter_with_base
  : public inserter<T>
{
  template <typename ManipValue>
  std::string format (ManipValue v)
  {
    this->writer << v;
    size_t size = this->writer.begin() - &this->view[0];
    return std::string{&this->view[0], size};
  }

  template <typename Manip>
  static std::string expected (T v, Manip manip)
  {
    std::ostringstream oss;
    oss << manip << v;
    return oss.str();
  }

  static std::string expected_bin (T v)
  {
    auto value = static_cast<uint64_t>(std::make_unsigned_t<T>(v));
    std::string result;
    do
    {
      result = ((value & 1) ? '1' : '0') + result;
    } while (value >>= 1);
    return result;
  }
};
TYPED_TEST_CASE_P(inserter_with_base);


TYPED_TEST_P(inserter_with_base, hex_min)
{
  EXPECT_EQ(
    this->expected(this->min(), std::hex),
    this->format(sal::hex(this->min()))
  );
}


TYPED_TEST_P(inserter_with_base, hex_between_min_zero)
{
  auto v = between(this->min(), this->zero());
  EXPECT_EQ(
    this->expected(v, std::hex),
    this->format(sal::hex(v))
  );
}


TYPED_TEST_P(inserter_with_base, hex_zero)
{
  EXPECT_EQ(
    this->expected(this->zero(), std::hex),
    this->format(sal::hex(this->zero()))
  );
}


TYPED_TEST_P(inserter_with_base, hex_between_zero_max)
{
  auto v = between(this->zero(), this->max());
  EXPECT_EQ(
    this->expected(v, std::hex),
    this->format(sal::hex(v))
  );
}


TYPED_TEST_P(inserter_with_base, hex_max)
{
  EXPECT_EQ(
    this->expected(this->min(), std::hex),
    this->format(sal::hex(this->min()))
  );
}


TYPED_TEST_P(inserter_with_base, oct_min)
{
  EXPECT_EQ(
    this->expected(this->min(), std::oct),
    this->format(sal::oct(this->min()))
  );
}


TYPED_TEST_P(inserter_with_base, oct_between_min_zero)
{
  auto v = between(this->min(), this->zero());
  EXPECT_EQ(
    this->expected(v, std::oct),
    this->format(sal::oct(v))
  );
}


TYPED_TEST_P(inserter_with_base, oct_zero)
{
  EXPECT_EQ(
    this->expected(this->zero(), std::oct),
    this->format(sal::oct(this->zero()))
  );
}


TYPED_TEST_P(inserter_with_base, oct_between_zero_max)
{
  auto v = between(this->zero(), this->max());
  EXPECT_EQ(
    this->expected(v, std::oct),
    this->format(sal::oct(v))
  );
}


TYPED_TEST_P(inserter_with_base, oct_max)
{
  EXPECT_EQ(
    this->expected(this->min(), std::oct),
    this->format(sal::oct(this->min()))
  );
}


TYPED_TEST_P(inserter_with_base, bin_min)
{
  EXPECT_EQ(
    this->expected_bin(this->min()),
    this->format(sal::bin(this->min()))
  );
}


TYPED_TEST_P(inserter_with_base, bin_between_min_zero)
{
  auto v = between(this->min(), this->zero());
  EXPECT_EQ(
    this->expected_bin(v),
    this->format(sal::bin(v))
  );
}


TYPED_TEST_P(inserter_with_base, bin_zero)
{
  EXPECT_EQ(
    this->expected_bin(this->zero()),
    this->format(sal::bin(this->zero()))
  );
}


TYPED_TEST_P(inserter_with_base, bin_between_zero_max)
{
  auto v = between(this->zero(), this->max());
  EXPECT_EQ(
    this->expected_bin(v),
    this->format(sal::bin(v))
  );
}


TYPED_TEST_P(inserter_with_base, bin_max)
{
  EXPECT_EQ(
    this->expected_bin(this->min()),
    this->format(sal::bin(this->min()))
  );
}


REGISTER_TYPED_TEST_CASE_P(member,
  ctor_array, ctor_range, ctor_empty_range, ctor_invalid_range, ctor_move,
  write, write_range, write_array,
  skip, skip_to_end, skip_past_end
);
using types = testing::Types<uint8_t, uint16_t, uint32_t, uint64_t, float>;
INSTANTIATE_TYPED_TEST_CASE_P(memory_writer, member, types);


REGISTER_TYPED_TEST_CASE_P(inserter,
  value_min,
  value_between_min_zero,
  value_zero,
  value_between_zero_max,
  value_max,
  range_empty,
  range_one_less,
  range_exact
);
using inserter_types = testing::Types<
  bool,
  char, signed char, unsigned char,
  short, unsigned short,
  int, unsigned int,
  long, unsigned long,
  long long, unsigned long long,
  float, double, long double
>;
INSTANTIATE_TYPED_TEST_CASE_P(memory_writer, inserter, inserter_types);


REGISTER_TYPED_TEST_CASE_P(inserter_with_base,
  hex_min, hex_between_min_zero, hex_zero, hex_between_zero_max, hex_max,
  oct_min, oct_between_min_zero, oct_zero, oct_between_zero_max, oct_max,
  bin_min, bin_between_min_zero, bin_zero, bin_between_zero_max, bin_max
);
using inserter_base_types = testing::Types<
  short, unsigned short,
  int, unsigned int,
  long, unsigned long,
  long long, unsigned long long
>;
INSTANTIATE_TYPED_TEST_CASE_P(memory_writer, inserter_with_base, inserter_base_types);


struct memory_writer
  : public sal_test::fixture
{
  char view[65] = "................................................................";
  sal::memory_writer_t writer{view};

  template <typename T>
  std::string format (T v)
  {
    writer << v;
    size_t size = writer.begin() - &view[0];
    return std::string{&view[0], size};
  }


  template <typename T>
  std::string expected (const T *p)
  {
    // gcc & msvc pointer formats differ, make it uniform here
    std::ostringstream oss;
    oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(p);
    return oss.str();
  }
};


TEST_F(memory_writer, nullptr)
{
  EXPECT_EQ("(null)", format(nullptr));
}


TEST_F(memory_writer, void_pointer)
{
  void *p = &p;
  EXPECT_EQ(expected(p), format(p));
}


TEST_F(memory_writer, void_pointer_null)
{
  void *p{};
  EXPECT_EQ(expected(p), format(p));
}


TEST_F(memory_writer, c_str)
{
  char buf[] = "abc";
  char *p = &buf[0];
  EXPECT_EQ(expected(p), format(p));
}


TEST_F(memory_writer, c_str_underflow)
{
  char v[] = "....";
  sal::memory_writer_t w{v};

  w << "123";
  EXPECT_TRUE(bool(w));
  EXPECT_FALSE(w.is_full());
  EXPECT_FALSE(w.is_bad());
  EXPECT_EQ(2U, w.size());

  EXPECT_STREQ("123.", v);
}


TEST_F(memory_writer, c_str_exact)
{
  char v[] = { '.', '.', '.', '\0' };
  sal::memory_writer_t w{v, v + 3};

  w << "123";
  EXPECT_TRUE(bool(w));
  EXPECT_TRUE(w.is_full());
  EXPECT_FALSE(w.is_bad());
  EXPECT_EQ(0U, w.size());

  EXPECT_STREQ("123", v);
}


TEST_F(memory_writer, c_str_overflow)
{
  char v[] = { '.', '.', '.', '\0' };
  sal::memory_writer_t w{v, v + 3};

  w << "1234";
  EXPECT_TRUE(bool(w));
  EXPECT_TRUE(w.is_full());
  EXPECT_FALSE(w.is_bad());
  EXPECT_EQ(0U, w.size());

  EXPECT_STREQ("123", v);
}


TEST_F(memory_writer, string)
{
  EXPECT_EQ(case_name, format(case_name));
}


class class_with_ostream_inserter
{
public:

  friend std::ostream &operator<< (std::ostream &os,
    const class_with_ostream_inserter &)
  {
    os << "(class_with_ostream_inserter)";
    return os;
  }
};


TEST_F(memory_writer, class_with_ostream_inserter)
{
  class_with_ostream_inserter v;
  std::ostringstream expected;
  expected << v;
  EXPECT_EQ(expected.str(), format(v));
}


class class_with_memory_writer_inserter
{
public:

  friend sal::memory_writer_t &operator<< (sal::memory_writer_t &w,
    const class_with_memory_writer_inserter &)
  {
    w << "(class_with_memory_writer_inserter)";
    return w;
  }
};


TEST_F(memory_writer, class_with_memory_stream_inserter)
{
  class_with_memory_writer_inserter v;
  EXPECT_EQ("(class_with_memory_writer_inserter)", format(v));
}


} // namespace
