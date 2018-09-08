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

  memory_writer (const memory_writer &) = delete;
  memory_writer &operator= (const memory_writer &) = delete;
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
  EXPECT_EQ(size, a.safe_size());
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
  EXPECT_EQ(size, a.safe_size());
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
  EXPECT_EQ(0U, a.safe_size());
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
  EXPECT_EQ(0U, a.safe_size());
}


TEST_F(memory_writer, ctor_move)
{
  auto a{std::move(writer)};

  EXPECT_TRUE(bool(a));
  EXPECT_TRUE(a.good());
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_EQ(size, a.size());
  EXPECT_EQ(size, a.safe_size());
  EXPECT_EQ(begin, a.begin());
  EXPECT_EQ(end, a.end());
}


TEST_F(memory_writer, assign_move)
{
  uint32_t d[1];
  sal::memory_writer_t a{d};
  a = std::move(writer);

  EXPECT_TRUE(bool(a));
  EXPECT_TRUE(a.good());
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_EQ(size, a.size());
  EXPECT_EQ(size, a.safe_size());
  EXPECT_EQ(begin, a.begin());
  EXPECT_EQ(end, a.end());
}


TEST_F(memory_writer, swap)
{
  char d[1];
  sal::memory_writer_t a{d};
  a.swap(writer);

  EXPECT_TRUE(bool(a));
  EXPECT_TRUE(a.good());
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_EQ(size, a.size());
  EXPECT_EQ(size, a.safe_size());
  EXPECT_EQ(begin, a.begin());
  EXPECT_EQ(end, a.end());

  EXPECT_TRUE(bool(writer));
  EXPECT_TRUE(writer.good());
  EXPECT_FALSE(writer.full());
  EXPECT_FALSE(writer.bad());

  EXPECT_EQ(sizeof(d), writer.size());
  EXPECT_EQ(d, writer.begin());
  EXPECT_EQ(d + sizeof(d), writer.end());
}


TEST_F(memory_writer, print)
{
  writer.print("hello", ',', ' ', case_name.c_str());
  EXPECT_EQ("hello, " + case_name, std::string(data, writer.first));
}


TEST_F(memory_writer, print_exact)
{
  sal::memory_writer_t a{data, data + 2};
  EXPECT_TRUE(bool(a.print('1')));
  EXPECT_TRUE(bool(a.print('2')));
  EXPECT_EQ("12.", std::string(data, 3));
}


TEST_F(memory_writer, print_overflow)
{
  sal::memory_writer_t a{data, data + 2};
  EXPECT_TRUE(bool(a.print('1')));
  EXPECT_TRUE(bool(a.print('2')));
  EXPECT_FALSE(bool(a.print('3')));
  EXPECT_EQ("12.", std::string(data, 3));
}


TEST_F(memory_writer, write)
{
  char expected = 1;
  EXPECT_TRUE(bool(writer.write(expected)));
  EXPECT_EQ(expected, *data);
  EXPECT_EQ(size - sizeof(expected), writer.size());
}


TEST_F(memory_writer, write_exact)
{
  char expected = 1;
  sal::memory_writer_t a{data, data + sizeof(expected)};
  EXPECT_TRUE(bool(a.write(expected)));
  EXPECT_TRUE(a.full());
  EXPECT_FALSE(a.bad());
  EXPECT_EQ(0U, a.size());
  EXPECT_EQ(0U, a.safe_size());
  EXPECT_EQ(expected, *data);
}


TEST_F(memory_writer, write_overflow)
{
  char expected = 1;
  sal::memory_writer_t a{data, data + sizeof(expected) / 2};
  EXPECT_FALSE(bool(a.write(expected)));
  EXPECT_TRUE(a.bad());
  EXPECT_EQ(0U, a.safe_size());
  EXPECT_EQ(
    std::string(sizeof(expected), '.'),
    std::string(data, sizeof(expected))
  );
}


TEST_F(memory_writer, write_range)
{
  char expected[] = "ab";
  size_t expected_size = sizeof(expected) - 1;
  EXPECT_TRUE(bool(writer.write(expected, expected + expected_size)));
  EXPECT_EQ(expected, std::string(data, expected_size));
  EXPECT_EQ(size - expected_size, writer.size());
}


TEST_F(memory_writer, write_range_exact)
{
  char expected[] = "ab";
  size_t expected_size = sizeof(expected) - 1;

  sal::memory_writer_t a{data, data + expected_size};
  EXPECT_TRUE(bool(a.write(expected, expected + expected_size)));
  EXPECT_TRUE(a.full());
  EXPECT_FALSE(a.bad());
  EXPECT_EQ(0U, a.size());
  EXPECT_EQ(expected, std::string(data, expected_size));
}


TEST_F(memory_writer, write_range_overflow)
{
  char expected[] = "ab";
  size_t expected_size = sizeof(expected) - 1;

  sal::memory_writer_t a{data, data + expected_size / 2};
  EXPECT_FALSE(bool(a.write(expected, expected + expected_size)));
  EXPECT_TRUE(a.bad());
  EXPECT_EQ(0U, a.safe_size());
  EXPECT_EQ(
    std::string(expected_size, '.'),
    std::string(data, expected_size)
  );
}


TEST_F(memory_writer, write_array)
{
  char expected[] = { 'a', 'b' };
  size_t expected_size = sizeof(expected);
  EXPECT_TRUE(bool(writer.write(expected)));
  EXPECT_EQ("ab", std::string(data, expected_size));
  EXPECT_EQ(size - expected_size, writer.size());
}


TEST_F(memory_writer, write_array_exact)
{
  char expected[] = { 'a', 'b' };
  size_t expected_size = sizeof(expected);

  sal::memory_writer_t a{data, data + expected_size};
  EXPECT_TRUE(bool(a.write(expected)));
  EXPECT_TRUE(a.full());
  EXPECT_FALSE(a.bad());
  EXPECT_EQ(0U, a.size());
  EXPECT_EQ("ab", std::string(data, expected_size));
}


TEST_F(memory_writer, write_array_overflow)
{
  char expected[] = { 'a', 'b' };
  size_t expected_size = sizeof(expected);

  sal::memory_writer_t a{data, data + expected_size / 2};
  EXPECT_FALSE(bool(a.write(expected)));
  EXPECT_TRUE(a.bad());
  EXPECT_EQ(0U, a.safe_size());
  EXPECT_EQ(
    std::string(expected_size, '.'),
    std::string(data, expected_size)
  );
}


TEST_F(memory_writer, skip)
{
  EXPECT_TRUE(bool(writer.skip(1)));
  EXPECT_TRUE(bool(writer.write('a')));
  EXPECT_EQ(".a", std::string(data, 2));
}


TEST_F(memory_writer, skip_exact)
{
  EXPECT_TRUE(bool(writer.skip(sizeof(data))));
  EXPECT_TRUE(writer.full());
  EXPECT_EQ(0U, writer.size());
  EXPECT_FALSE(bool(writer.write('a')));
  EXPECT_TRUE(writer.bad());
  EXPECT_EQ(0U, writer.safe_size());
  EXPECT_EQ(std::string(size, '.'), std::string(data, size));
}


TEST_F(memory_writer, skip_overflow)
{
  EXPECT_FALSE(bool(writer.skip(2 * sizeof(data))));
  EXPECT_TRUE(writer.bad());
  EXPECT_EQ(0U, writer.safe_size());
  EXPECT_EQ(std::string(size, '.'), std::string(data, size));
}


TEST_F(memory_writer, skip_until)
{
  data[1] = 'a';
  EXPECT_TRUE(bool(writer.skip_until('a')));
  EXPECT_EQ(sizeof(data) - 1, writer.size());
}


TEST_F(memory_writer, skip_until_exact)
{
  data[sizeof(data) - 1] = 'a';
  EXPECT_TRUE(bool(writer.skip_until('a')));
  EXPECT_EQ(1U, writer.size());
}


TEST_F(memory_writer, skip_until_overflow)
{
  EXPECT_TRUE(bool(writer.skip_until('a')));
  EXPECT_EQ(0U, writer.size());
}


TEST_F(memory_writer, inserter_c_str)
{
  char expected[] = "123";
  EXPECT_TRUE(bool(writer << expected));
  EXPECT_EQ("123.", std::string(data, data + 4));
}


TEST_F(memory_writer, inserter_c_str_exact)
{
  char expected[] = "123";
  sal::memory_writer_t w{data, data + sizeof(expected) - 1};
  EXPECT_TRUE(bool(w << expected));
  EXPECT_TRUE(w.full());
  EXPECT_EQ("123.", std::string(data, data + 4));
}


TEST_F(memory_writer, inserter_c_str_one_char_more)
{
  char expected[] = "123";
  sal::memory_writer_t w{data, data + sizeof(expected)};
  EXPECT_TRUE(bool(w << expected));
  EXPECT_FALSE(w.full());
  EXPECT_EQ("123.", std::string(data, data + 4));
}


TEST_F(memory_writer, inserter_c_str_one_char_less)
{
  char expected[] = "123";
  sal::memory_writer_t w{data, data + sizeof(expected) - 2};
  EXPECT_FALSE(bool(w << expected));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ(0U, w.safe_size());
  EXPECT_EQ("12..", std::string(data, data + sizeof(expected)));
}


TEST_F(memory_writer, inserter_c_str_overflow)
{
  char expected[] = "12345";
  sal::memory_writer_t w{data, data + sizeof(expected) / 2};
  EXPECT_FALSE(bool(w << expected));
  EXPECT_TRUE(w.bad());
  EXPECT_EQ(0U, w.safe_size());
  EXPECT_EQ("123...", std::string(data, data + sizeof(expected)));
}


template <typename T>
using inserter = sal_test::with_type<T>;
using char_types = testing::Types<char, signed char, unsigned char>;
TYPED_TEST_CASE(inserter, char_types, );


TYPED_TEST(inserter, character)
{
  char d[2];
  TypeParam expected = 'a';
  sal::memory_writer_t w{d};
  EXPECT_TRUE(bool(w << expected));
  EXPECT_EQ(expected, d[0]);
}


TYPED_TEST(inserter, character_exact)
{
  char d{};
  TypeParam expected = 'a';
  sal::memory_writer_t w{&d, &d + sizeof(d)};
  EXPECT_TRUE(bool(w << expected));
  EXPECT_TRUE(w.full());
  EXPECT_EQ(expected, d);
}


TYPED_TEST(inserter, character_overflow)
{
  char d{};
  TypeParam expected = 'a';
  sal::memory_writer_t w{&d, &d};
  EXPECT_FALSE(bool(w << expected));
  EXPECT_EQ('\0', d);
}


} // namespace
