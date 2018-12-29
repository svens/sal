#include <sal/memory.hpp>
#include <sal/common.test.hpp>
#include <array>
#include <vector>


namespace {


using memory = sal_test::fixture;


TEST_F(memory, to_ptr_nullptr)
{
  EXPECT_EQ(nullptr, sal::to_ptr(nullptr));
}


TEST_F(memory, range_size_nullptr)
{
  EXPECT_EQ(0U, sal::range_size(nullptr, nullptr));
}


TEST_F(memory, to_end_ptr_nullptr)
{
  EXPECT_EQ(nullptr, sal::to_end_ptr(nullptr, nullptr));
}


TEST_F(memory, to_ptr_char_array)
{
  uint8_t data[] = { 1 };

  auto p = sal::to_ptr(data);
  EXPECT_EQ(reinterpret_cast<uint8_t *>(&data[0]), p);

  *p = 2;
  EXPECT_EQ(2, data[0]);

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data + 1), p);
}


TEST_F(memory, to_ptr_int_array)
{
  int data[] = { 1 };
  auto p = sal::to_ptr(data);
  EXPECT_EQ(reinterpret_cast<uint8_t *>(&data[0]), p);

  *p = 2;
  EXPECT_NE(1, data[0]);

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ(reinterpret_cast<uint8_t *>(&data[1]), p);
}


TEST_F(memory, to_ptr_const_char_array)
{
  const uint8_t data[] = { 1 };
  auto p = sal::to_ptr(data);
  EXPECT_EQ(reinterpret_cast<const uint8_t *>(&data[0]), p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ(reinterpret_cast<const uint8_t *>(data + 1), p);
}


TEST_F(memory, to_ptr_const_int_array)
{
  const int data[] = { 1 };
  auto p = sal::to_ptr(data);
  EXPECT_EQ(reinterpret_cast<const uint8_t *>(&data[0]), p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ(reinterpret_cast<const uint8_t *>(&data[1]), p);
}


TEST_F(memory, to_ptr_char_ptr)
{
  uint8_t data[] = { 1 };
  auto *ptr = data;

  auto p = sal::to_ptr(ptr);
  EXPECT_EQ(reinterpret_cast<uint8_t *>(&data[0]), p);

  *p = 2;
  EXPECT_EQ(2, data[0]);

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ(reinterpret_cast<uint8_t *>(&data[1]), p);
}


TEST_F(memory, to_ptr_int_ptr)
{
  int data[] = { 1 };
  auto *ptr = data;

  auto p = sal::to_ptr(ptr);
  EXPECT_EQ(reinterpret_cast<uint8_t *>(&data[0]), p);

  *p = 2;
  EXPECT_NE(1, data[0]);

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ(reinterpret_cast<uint8_t *>(&data[1]), p);
}


TEST_F(memory, to_ptr_const_char_ptr)
{
  const uint8_t data[] = { 1 };
  auto *ptr = data;

  auto p = sal::to_ptr(ptr);
  EXPECT_EQ(reinterpret_cast<const uint8_t *>(&data[0]), p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ(reinterpret_cast<const uint8_t *>(&data[1]), p);
}


TEST_F(memory, to_ptr_const_int_ptr)
{
  const int data[] = { 1 };
  auto *ptr = data;

  auto p = sal::to_ptr(ptr);
  EXPECT_EQ(reinterpret_cast<const uint8_t *>(&data[0]), p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data, data + 1);
  EXPECT_EQ(reinterpret_cast<const uint8_t *>(&data[1]), p);
}


TEST_F(memory, to_ptr_array_begin)
{
  std::array<uint8_t, 1> data = {{ 1 }};

  auto p = sal::to_ptr(data.begin());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data()), p);

  *p = 2;
  EXPECT_EQ(2, data[0]);

  p = sal::to_end_ptr(data.begin(), data.end());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data() + 1), p);
}


TEST_F(memory, to_ptr_int_array_begin)
{
  std::array<int, 1> data = {{ 1 }};

  auto p = sal::to_ptr(data.begin());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data()), p);

  *p = 2;
  EXPECT_NE(1, data[0]);

  p = sal::to_end_ptr(data.begin(), data.end());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data() + 1), p);
}


TEST_F(memory, to_ptr_array_cbegin)
{
  std::array<uint8_t, 1> data = {{ 1 }};

  auto p = sal::to_ptr(data.cbegin());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data()), p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data.cbegin(), data.cend());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data() + 1), p);
}


TEST_F(memory, to_ptr_int_array_cbegin)
{
  std::array<int, 1> data = {{ 1 }};

  auto p = sal::to_ptr(data.cbegin());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data()), p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data.cbegin(), data.cend());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data() + 1), p);
}


TEST_F(memory, to_ptr_vector_begin)
{
  std::vector<uint8_t> data = { 1 };

  auto p = sal::to_ptr(data.begin());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data()), p);

  *p = 2;
  EXPECT_EQ(2, data[0]);

  p = sal::to_end_ptr(data.begin(), data.end());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data() + 1), p);
}


TEST_F(memory, to_ptr_int_vector_begin)
{
  std::vector<int> data = { 1 };

  auto p = sal::to_ptr(data.begin());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data()), p);

  *p = 2;
  EXPECT_NE(1, data[0]);

  p = sal::to_end_ptr(data.begin(), data.end());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data() + 1), p);
}


TEST_F(memory, to_ptr_vector_cbegin)
{
  std::vector<uint8_t> data = { 1 };

  auto p = sal::to_ptr(data.cbegin());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data()), p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data.cbegin(), data.cend());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data() + 1), p);
}


TEST_F(memory, to_ptr_int_vector_cbegin)
{
  std::vector<int> data = { 1 };

  auto p = sal::to_ptr(data.cbegin());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data()), p);

  // assignment of ready-only location
  // *p = 2;

  p = sal::to_end_ptr(data.cbegin(), data.cend());
  EXPECT_EQ(reinterpret_cast<uint8_t *>(data.data() + 1), p);
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


TEST_F(memory, as_view_from_ptr_and_size)
{
  auto data = case_name;
  auto view = sal::as_view(&case_name[0], case_name.size());
  EXPECT_EQ(&case_name[0], view.data());
  EXPECT_EQ(case_name.size(), view.size());
  EXPECT_EQ(case_name, view);
}


TEST_F(memory, as_view_from_range)
{
  auto view = sal::as_view(&case_name[0], &case_name[case_name.size()]);
  EXPECT_EQ(&case_name[0], view.data());
  EXPECT_EQ(case_name.size(), view.size());
  EXPECT_EQ(case_name, view);
}


TEST_F(memory, as_view_from_c_str)
{
  auto view = sal::as_view(case_name.c_str());
  EXPECT_EQ(&case_name[0], view.data());
  EXPECT_EQ(case_name.size(), view.size());
  EXPECT_EQ(case_name, view);
}


TEST_F(memory, as_view_from_string)
{
  auto view = sal::as_view(case_name);
  EXPECT_EQ(&case_name[0], view.data());
  EXPECT_EQ(case_name.size(), view.size());
  EXPECT_EQ(case_name, view);
}


} // namespace
