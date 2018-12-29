#include <sal/span.hpp>
#include <sal/common.test.hpp>
#include <vector>


namespace {


template <typename T>
struct span
  : public sal_test::with_type<T>
{
  static T array[3];
  static constexpr size_t count = sizeof(array) / sizeof(array[0]);
  static constexpr size_t bytes = count * sizeof(T);

  static std::vector<T> std_vector;
  static std::array<T, count> std_array;
};

template <typename T>
T span<T>::array[] = {1, 2, 3};

template <typename T>
std::vector<T> span<T>::std_vector{array, array + count};

template <typename T>
std::array<T, span<T>::count> span<T>::std_array{1, 2, 3};

using pod_types = ::testing::Types<
  uint8_t,
  uint16_t,
  uint32_t,
  uint64_t
>;

struct pod_type_names
{
  template <typename T>
  static std::string GetName (int i)
  {
    (void)i;
    if constexpr (std::is_same_v<T, uint8_t>)
    {
      return "uint8_t";
    }
    else if constexpr (std::is_same_v<T, uint16_t>)
    {
      return "uint16_t";
    }
    else if constexpr (std::is_same_v<T, uint32_t>)
    {
      return "uint32_t";
    }
    else if constexpr (std::is_same_v<T, uint64_t>)
    {
      return "uint64_t";
    }
    else
    {
      return std::to_string(i);
    }
  }
};

TYPED_TEST_CASE(span, pod_types, pod_type_names);


TYPED_TEST(span, empty)
{
  sal::span_t<TypeParam> span;
  EXPECT_TRUE(span.empty());
  EXPECT_EQ(nullptr, span.data());
  EXPECT_EQ(0U, span.size());
  EXPECT_EQ(0U, span.size_bytes());
}


TYPED_TEST(span, with_ptr_and_count)
{
  auto span = sal::span(this->array, this->count);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->array, span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, with_const_ptr_and_count)
{
  auto span = sal::const_span(this->array, this->count);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->array, span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, with_range)
{
  auto span = sal::span(this->array, this->array + this->count);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->array, span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, with_const_range)
{
  auto span = sal::const_span(this->array, this->array + this->count);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->array, span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, with_array)
{
  auto span = sal::span(this->array);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->array, span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, with_const_array)
{
  auto span = sal::const_span(this->array);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->array, span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, with_std_vector)
{
  auto span = sal::span(this->std_vector);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->std_vector.data(), span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, with_const_std_vector)
{
  auto span = sal::const_span(this->std_vector);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->std_vector.data(), span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, with_std_array)
{
  auto span = sal::span(this->std_array);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->std_array.data(), span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, with_const_std_array)
{
  auto span = sal::const_span(this->std_array);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->std_array.data(), span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, index)
{
  std::vector<TypeParam> data;
  auto span = sal::const_span(this->std_vector);
  for (auto index = 0U;  index < span.size();  ++index)
  {
    data.emplace_back(span[index]);
  }
  EXPECT_EQ(this->std_vector, data);
}


TYPED_TEST(span, iterator)
{
  std::vector<TypeParam> data;
  auto span = sal::span(this->std_vector);
  for (auto it = span.begin();  it != span.end();  ++it)
  {
    data.emplace_back(*it);
  }
  EXPECT_EQ(this->std_vector, data);
}


TYPED_TEST(span, const_iterator)
{
  std::vector<TypeParam> data;
  auto span = sal::const_span(this->std_vector);
  for (auto it = span.cbegin();  it != span.cend();  ++it)
  {
    data.emplace_back(*it);
  }
  EXPECT_EQ(this->std_vector, data);
}


TYPED_TEST(span, reverse_iterator)
{
  std::vector<TypeParam> data;
  auto span = sal::span(this->std_vector);
  for (auto it = span.rbegin();  it != span.rend();  ++it)
  {
    data.emplace_back(*it);
  }

  auto reverse_data = this->std_vector;
  std::reverse(reverse_data.begin(), reverse_data.end());
  EXPECT_EQ(reverse_data, data);
}


TYPED_TEST(span, const_reverse_iterator)
{
  std::vector<TypeParam> data;
  auto span = sal::const_span(this->std_vector);
  for (auto it = span.crbegin();  it != span.crend();  ++it)
  {
    data.emplace_back(*it);
  }

  auto reverse_data = this->std_vector;
  std::reverse(reverse_data.begin(), reverse_data.end());
  EXPECT_EQ(reverse_data, data);
}


TYPED_TEST(span, as_bytes)
{
  auto span = sal::as_bytes(sal::const_span(this->array));
  EXPECT_TRUE(std::is_const_v<typename decltype(span)::element_type>);
  EXPECT_EQ(
    reinterpret_cast<const std::byte *>(this->array),
    span.data()
  );
}


TYPED_TEST(span, as_writable_bytes)
{
  auto span = sal::as_writable_bytes(sal::span(this->array));
  EXPECT_EQ(
    reinterpret_cast<std::byte *>(this->array),
    span.data()
  );
}


} // namespace
