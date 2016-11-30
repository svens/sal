#include <sal/memory_writer.hpp>
#include <sal/common.test.hpp>


namespace {


template <typename T>
using test = sal_test::with_type<T>;
using types = testing::Types<char, short, int, long, float, double>;

TYPED_TEST_CASE_P(test);


TYPED_TEST_P(test, ctor_array)
{
  TypeParam d[32];
  sal::memory_writer_t a{d};

  ASSERT_TRUE(bool(a));
  EXPECT_FALSE(a.full());
  EXPECT_EQ(sizeof(d), a.size());

  auto expected = reinterpret_cast<char *>(&d[0]);
  EXPECT_EQ(expected, a.begin());
  EXPECT_EQ(expected + sizeof(d), a.end());
}


TYPED_TEST_P(test, ctor_range)
{
  TypeParam d[32];
  sal::memory_writer_t a{d, d + 32};

  ASSERT_TRUE(bool(a));
  EXPECT_FALSE(a.full());
  EXPECT_EQ(sizeof(d), a.size());

  auto expected = reinterpret_cast<char *>(&d[0]);
  EXPECT_EQ(expected, a.begin());
  EXPECT_EQ(expected + sizeof(d), a.end());
}


TYPED_TEST_P(test, ctor_empty_range)
{
  TypeParam d[32];
  sal::memory_writer_t a{d, d};
  EXPECT_FALSE(bool(a));
}


TYPED_TEST_P(test, ctor_invalid_range)
{
  TypeParam d[32];
  sal::memory_writer_t a{d + 32, d};
  EXPECT_FALSE(bool(a));
}


TYPED_TEST_P(test, ctor_move)
{
  TypeParam d[32];
  sal::memory_writer_t a{d};
  EXPECT_TRUE(bool(a));

  auto b{std::move(a)};
  EXPECT_TRUE(bool(b));
  EXPECT_FALSE(b.full());
  EXPECT_EQ(sizeof(d), b.size());

  auto expected = reinterpret_cast<char *>(&d[0]);
  EXPECT_EQ(expected, b.begin());
  EXPECT_EQ(expected + sizeof(d), b.end());

  EXPECT_FALSE(bool(a));
}


TYPED_TEST_P(test, write)
{
  TypeParam d[] = { {}, {} };
  sal::memory_writer_t a{d};

  ASSERT_TRUE(bool(a));
  TypeParam expected = d[0] + 1;
  a.write(expected);
  EXPECT_EQ(expected, d[0]);

  ASSERT_TRUE(bool(a));
  expected = d[1] + 2;
  a.write(expected);
  EXPECT_EQ(expected, d[1]);

  EXPECT_FALSE(bool(a));
}


TYPED_TEST_P(test, try_write)
{
  TypeParam d{};
  sal::memory_writer_t a{&d, &d + 1};

  TypeParam expected = d + 1;
  EXPECT_TRUE(a.try_write(expected));
  EXPECT_EQ(expected, d);

  expected = d + 2;
  EXPECT_FALSE(a.try_write(expected));
  EXPECT_NE(expected, d);

  EXPECT_FALSE(bool(a));
}


TYPED_TEST_P(test, write_range)
{
  TypeParam d[] = { {}, {}, {} };
  sal::memory_writer_t a{d};
  ASSERT_TRUE(bool(a));

  TypeParam expected[] = { {}, {} };
  expected[0]++, expected[1]++, expected[1]++;
  a.write(expected, expected + 2);
  EXPECT_EQ(expected[0], d[0]);
  EXPECT_EQ(expected[1], d[1]);

  ASSERT_TRUE(bool(a));
  a.write(expected, expected + 1);
  EXPECT_EQ(expected[0], d[2]);

  EXPECT_FALSE(bool(a));
}


TYPED_TEST_P(test, try_write_range)
{
  TypeParam d[] = { {}, {}, {} };
  sal::memory_writer_t a{d};
  ASSERT_TRUE(bool(a));

  TypeParam expected[] = { {}, {} };
  expected[0]++, expected[1]++, expected[1]++;
  EXPECT_TRUE(a.try_write(expected, expected + 2));
  EXPECT_EQ(expected[0], d[0]);
  EXPECT_EQ(expected[1], d[1]);

  EXPECT_FALSE(a.try_write(expected, expected + 2));
  EXPECT_EQ(TypeParam{}, d[2]);

  EXPECT_TRUE(bool(a));
}


TYPED_TEST_P(test, write_array)
{
  TypeParam d[] = { {}, {}, {} };
  sal::memory_writer_t a{d};
  ASSERT_TRUE(bool(a));

  TypeParam expected[] = { {}, {} };
  expected[0]++, expected[1]++, expected[1]++;
  a.write(expected);
  EXPECT_EQ(expected[0], d[0]);
  EXPECT_EQ(expected[1], d[1]);

  ASSERT_TRUE(bool(a));
  a.write(expected[0]);
  EXPECT_EQ(expected[0], d[2]);

  EXPECT_FALSE(bool(a));
}


TYPED_TEST_P(test, try_write_array)
{
  TypeParam d[] = { {}, {}, {} };
  sal::memory_writer_t a{d};
  ASSERT_TRUE(bool(a));

  TypeParam expected[] = { {}, {} };
  expected[0]++, expected[1]++, expected[1]++;
  EXPECT_TRUE(a.try_write(expected));
  EXPECT_EQ(expected[0], d[0]);
  EXPECT_EQ(expected[1], d[1]);

  EXPECT_FALSE(a.try_write(expected));
  EXPECT_EQ(TypeParam{}, d[2]);

  EXPECT_TRUE(bool(a));
}


TYPED_TEST_P(test, skip)
{
  TypeParam d[] = { {}, {} };
  sal::memory_writer_t a{d};

  ASSERT_TRUE(bool(a));
  EXPECT_TRUE(bool(a.skip(sizeof(TypeParam))));
  EXPECT_EQ(TypeParam{}, d[0]);

  TypeParam expected = d[1] + 1;
  a.write(expected);
  EXPECT_EQ(expected, d[1]);
  EXPECT_EQ(TypeParam{}, d[0]);

  EXPECT_FALSE(bool(a));
}


TYPED_TEST_P(test, skip_past_end)
{
  TypeParam d[] = { {}, {} };
  sal::memory_writer_t a{d};

  ASSERT_TRUE(bool(a));
  EXPECT_FALSE(bool(a.skip(2*sizeof(TypeParam))));
  EXPECT_EQ(TypeParam{}, d[0]);
  EXPECT_EQ(TypeParam{}, d[1]);

  EXPECT_FALSE(bool(a));
}


REGISTER_TYPED_TEST_CASE_P(test,
  ctor_array,
  ctor_range,
  ctor_empty_range,
  ctor_invalid_range,
  ctor_move,
  write,
  try_write,
  write_range,
  try_write_range,
  write_array,
  try_write_array,
  skip,
  skip_past_end
);

INSTANTIATE_TYPED_TEST_CASE_P(memory_writer, test, types);


} // namespace
