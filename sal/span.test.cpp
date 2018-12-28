#include <sal/span.hpp>
#include <sal/common.test.hpp>
#include <vector>


namespace {


template <typename T>
struct span
  : public sal_test::with_type<T>
{
  static T arr[3];
  static constexpr std::ptrdiff_t count = sizeof(arr) / sizeof(arr[0]);
  static constexpr std::ptrdiff_t bytes = count * sizeof(T);
  static std::vector<T> vector;
};

template <typename T>
T span<T>::arr[] = { 1, 2, 3 };

template <typename T>
std::vector<T> span<T>::vector{arr, arr + count};

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


TYPED_TEST(span, ctor)
{
  sal::span_t<TypeParam> span;
  EXPECT_TRUE(span.empty());
  EXPECT_EQ(nullptr, span.data());
  EXPECT_EQ(0U, span.size());
  EXPECT_EQ(0U, span.size_bytes());
}


TYPED_TEST(span, ctor_with_ptr_and_count)
{
  sal::span_t<TypeParam> span(this->arr, this->count);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->arr, span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, ctor_with_range)
{
  sal::span_t<TypeParam> span(this->arr, this->arr + this->count);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->arr, span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, ctor_with_array)
{
  sal::span_t<TypeParam> span(this->arr);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->arr, span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, ctor_with_vector)
{
  sal::span_t<TypeParam> span(this->vector);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->vector.data(), span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, ctor_with_const_vector)
{
  const auto &data = this->vector;
  sal::span_t<const TypeParam> span(data);
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(this->vector.data(), span.data());
  EXPECT_EQ(this->count, span.size());
  EXPECT_EQ(this->bytes, span.size_bytes());
}


TYPED_TEST(span, index)
{
  std::vector<TypeParam> data;

  sal::span_t<const TypeParam> span(this->vector);
  for (auto index = 0;  index < span.size();  ++index)
  {
    data.emplace_back(span[index]);
  }

  EXPECT_EQ(this->vector, data);
}


TYPED_TEST(span, iterator)
{
  std::vector<TypeParam> data;

  sal::span_t<const TypeParam> span(this->vector);
  for (auto it = span.begin();  it != span.end();  ++it)
  {
    data.emplace_back(*it);
  }

  EXPECT_EQ(this->vector, data);
}


TYPED_TEST(span, const_iterator)
{
  std::vector<TypeParam> data;

  sal::span_t<const TypeParam> span(this->vector);
  for (auto it = span.cbegin();  it != span.cend();  ++it)
  {
    data.emplace_back(*it);
  }

  EXPECT_EQ(this->vector, data);
}


TYPED_TEST(span, as_bytes)
{
  sal::span_t<TypeParam> span(this->arr);
  auto data = sal::as_bytes(span);
  EXPECT_TRUE(std::is_const_v<typename decltype(data)::element_type>);
  EXPECT_EQ(
    reinterpret_cast<const std::byte *>(this->arr),
    data.data()
  );
}


TYPED_TEST(span, as_writable_bytes)
{
  sal::span_t<TypeParam> span(this->arr);
  auto data = sal::as_writable_bytes(span);
  EXPECT_EQ(
    reinterpret_cast<std::byte *>(this->arr),
    data.data()
  );
}


} // namespace
