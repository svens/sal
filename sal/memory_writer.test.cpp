#include <sal/memory_writer.hpp>
#include <sal/common.test.hpp>


namespace {


constexpr size_t count ()
{
  return 32;
}


struct memory_writer
  : public sal_test::fixture
{
  uint32_t data[count()];

  size_t size () const noexcept
  {
    return sizeof(data);
  }

  const char *begin () const noexcept
  {
    return reinterpret_cast<const char *>(data);
  }

  const char *end () const noexcept
  {
    return begin() + size();
  }
};


TEST_F(memory_writer, ctor_range)
{
  sal::memory_writer_t a{data, data + count()};

  EXPECT_TRUE(bool(a));
  EXPECT_TRUE(a.good());
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_EQ(size(), a.size());
  EXPECT_EQ(begin(), a.begin());
  EXPECT_EQ(end(), a.end());
}


TEST_F(memory_writer, ctor_array)
{
  sal::memory_writer_t a{data};

  EXPECT_TRUE(bool(a));
  EXPECT_TRUE(a.good());
  EXPECT_FALSE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_EQ(size(), a.size());
  EXPECT_EQ(begin(), a.begin());
  EXPECT_EQ(end(), a.end());
}


TEST_F(memory_writer, ctor_empty_range)
{
  sal::memory_writer_t a{data, data};

  EXPECT_TRUE(bool(a));
  EXPECT_TRUE(a.good());
  EXPECT_TRUE(a.full());
  EXPECT_FALSE(a.bad());

  EXPECT_EQ(0U, a.size());
  EXPECT_EQ(begin(), a.begin());
  EXPECT_EQ(begin(), a.end());
}


TEST_F(memory_writer, ctor_invalid_range)
{
  sal::memory_writer_t a{data + count(), data};
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

  EXPECT_FALSE(bool(a));
  EXPECT_FALSE(a.good());
  EXPECT_FALSE(a.full());
  EXPECT_TRUE(a.bad());
}


TEST_F(memory_writer, assign_move)
{
  sal::memory_writer_t a{data};
  EXPECT_TRUE(bool(a));
  EXPECT_EQ(begin(), a.begin());
  EXPECT_EQ(end(), a.end());

  uint32_t d[1];
  sal::memory_writer_t b{d};
  EXPECT_TRUE(bool(b));
  auto expected = reinterpret_cast<char *>(&d[0]);
  EXPECT_EQ(expected, b.begin());
  EXPECT_EQ(expected + sizeof(d), b.end());

  b = std::move(a);
  EXPECT_TRUE(bool(b));
  EXPECT_EQ(begin(), b.begin());
  EXPECT_EQ(end(), b.end());

  EXPECT_TRUE(a.bad());
}


TEST_F(memory_writer, swap)
{
  sal::memory_writer_t a{data};
  EXPECT_TRUE(bool(a));
  EXPECT_EQ(begin(), a.begin());
  EXPECT_EQ(end(), a.end());

  uint32_t d[1];
  sal::memory_writer_t b{d};
  EXPECT_TRUE(bool(b));
  auto expected = reinterpret_cast<char *>(&d[0]);
  EXPECT_EQ(expected, b.begin());
  EXPECT_EQ(expected + sizeof(d), b.end());

  a.swap(b);

  EXPECT_TRUE(bool(b));
  EXPECT_EQ(begin(), b.begin());
  EXPECT_EQ(end(), b.end());

  EXPECT_TRUE(bool(a));
  EXPECT_EQ(expected, a.begin());
  EXPECT_EQ(expected + sizeof(d), a.end());
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


REGISTER_TYPED_TEST_CASE_P(write,
  single, range, array
);
REGISTER_TYPED_TEST_CASE_P(skip,
  basic, to_end, past_end
);

using types = testing::Types<uint8_t, uint16_t, uint32_t, uint64_t, float>;
INSTANTIATE_TYPED_TEST_CASE_P(memory_writer, write, types);
INSTANTIATE_TYPED_TEST_CASE_P(memory_writer, skip, types);


} // namespace
