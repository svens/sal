#include <sal/memory.hpp>
#include <sal/common.test.hpp>
#include <array>
#include <vector>


namespace {


using memory = sal_test::fixture;


TEST_F(memory, to_ptr_char_array)
{
  uint8_t data[] = { 1 };

  auto p = sal::to_ptr(data);
  EXPECT_EQ(&data[0], p);

  *p = 2;
  EXPECT_EQ(2, data[0]);

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ(data + 1, p);
}


TEST_F(memory, to_ptr_int_array)
{
  int data[] = { 1 };
  auto p = sal::to_ptr(data);
  EXPECT_EQ((void *)&data[0], (void *)p);

  *p = 2;
  EXPECT_NE(1, data[0]);

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ((void *)(data + 1), p);
}


TEST_F(memory, to_ptr_const_char_array)
{
  const uint8_t data[] = { 1 };
  auto p = sal::to_ptr(data);
  EXPECT_EQ(&data[0], p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ((void *)(data + 1), p);
}


TEST_F(memory, to_ptr_const_int_array)
{
  const int data[] = { 1 };
  auto p = sal::to_ptr(data);
  EXPECT_EQ((void *)&data[0], (void *)p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ((void *)(data + 1), p);
}


TEST_F(memory, to_ptr_char_ptr)
{
  uint8_t data[] = { 1 };
  auto *ptr = data;

  auto p = sal::to_ptr(ptr);
  EXPECT_EQ(&data[0], p);

  *p = 2;
  EXPECT_EQ(2, data[0]);

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ((void *)(data + 1), p);
}


TEST_F(memory, to_ptr_int_ptr)
{
  int data[] = { 1 };
  auto *ptr = data;

  auto p = sal::to_ptr(ptr);
  EXPECT_EQ((void *)&data[0], (void *)p);

  *p = 2;
  EXPECT_NE(1, data[0]);

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ((void *)(data + 1), p);
}


TEST_F(memory, to_ptr_const_char_ptr)
{
  const uint8_t data[] = { 1 };
  auto *ptr = data;

  auto p = sal::to_ptr(ptr);
  EXPECT_EQ(&data[0], p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ((void *)(data + 1), p);
}


TEST_F(memory, to_ptr_const_int_ptr)
{
  const int data[] = { 1 };
  auto *ptr = data;

  auto p = sal::to_ptr(ptr);
  EXPECT_EQ((void *)&data[0], (void *)p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ((void *)(data + 1), p);
}


TEST_F(memory, to_ptr_array_begin)
{
  std::array<uint8_t, 1> data = {{ 1 }};

  auto p = sal::to_ptr(data.begin());
  EXPECT_EQ(data.data(), p);

  *p = 2;
  EXPECT_EQ(2, data[0]);

  p = sal::to_end_ptr(data.begin(), data.end());
  EXPECT_EQ((void *)(data.data() + 1), p);
}


TEST_F(memory, to_ptr_int_array_begin)
{
  std::array<int, 1> data = {{ 1 }};

  auto p = sal::to_ptr(data.begin());
  EXPECT_EQ((void *)data.data(), (void *)p);

  *p = 2;
  EXPECT_NE(1, data[0]);

  p = sal::to_end_ptr(data.begin(), data.end());
  EXPECT_EQ((void *)(data.data() + 1), p);
}


TEST_F(memory, to_ptr_array_cbegin)
{
  std::array<uint8_t, 1> data = {{ 1 }};

  auto p = sal::to_ptr(data.cbegin());
  EXPECT_EQ(data.data(), p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data.cbegin(), data.cend());
  EXPECT_EQ((void *)(data.data() + 1), p);
}


TEST_F(memory, to_ptr_int_array_cbegin)
{
  std::array<int, 1> data = {{ 1 }};

  auto p = sal::to_ptr(data.cbegin());
  EXPECT_EQ((void *)data.data(), (void *)p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data.cbegin(), data.cend());
  EXPECT_EQ((void *)(data.data() + 1), p);
}


TEST_F(memory, to_ptr_vector_begin)
{
  std::vector<uint8_t> data = { 1 };

  auto p = sal::to_ptr(data.begin());
  EXPECT_EQ(data.data(), p);

  *p = 2;
  EXPECT_EQ(2, data[0]);

  p = sal::to_end_ptr(data.begin(), data.end());
  EXPECT_EQ((void *)(data.data() + 1), p);
}


TEST_F(memory, to_ptr_int_vector_begin)
{
  std::vector<int> data = { 1 };

  auto p = sal::to_ptr(data.begin());
  EXPECT_EQ((void *)data.data(), (void *)p);

  *p = 2;
  EXPECT_NE(1, data[0]);

  p = sal::to_end_ptr(data.begin(), data.end());
  EXPECT_EQ((void *)(data.data() + 1), p);
}


TEST_F(memory, to_ptr_vector_cbegin)
{
  std::vector<uint8_t> data = { 1 };

  auto p = sal::to_ptr(data.cbegin());
  EXPECT_EQ(data.data(), p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data.cbegin(), data.cend());
  EXPECT_EQ((void *)(data.data() + 1), p);
}


TEST_F(memory, to_ptr_int_vector_cbegin)
{
  std::vector<int> data = { 1 };

  auto p = sal::to_ptr(data.cbegin());
  EXPECT_EQ((void *)data.data(), (void *)p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data.cbegin(), data.cend());
  EXPECT_EQ((void *)(data.data() + 1), p);
}


TEST_F(memory, range_size_char_array)
{
  char data[] = { 1, 2 };
  auto *first = &data[0], *last = first + 2;
  EXPECT_EQ(2 * sizeof(data[0]), sal::range_size(first, last));
}


TEST_F(memory, range_size_int_array)
{
  int data[] = { 1, 2 };
  auto *first = &data[0], *last = first + 2;
  EXPECT_EQ(2 * sizeof(data[0]), sal::range_size(first, last));
}


TEST_F(memory, range_size_char_vector)
{
  std::vector<char> data = { 1, 2 };
  EXPECT_EQ(2 * sizeof(data[0]), sal::range_size(data.begin(), data.end()));
}


TEST_F(memory, range_size_int_vector)
{
  std::vector<int> data = { 1, 2 };
  EXPECT_EQ(2 * sizeof(data[0]), sal::range_size(data.begin(), data.end()));
}


TEST_F(memory, range_size_empty_vector)
{
  std::vector<char> data;
  EXPECT_EQ(0U, sal::range_size(data.begin(), data.end()));
}


} // namespace
