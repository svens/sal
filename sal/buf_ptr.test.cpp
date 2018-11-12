#include <sal/buf_ptr.hpp>
#include <sal/common.test.hpp>


namespace {


constexpr size_t buf_size = 4;


template <typename T>
auto *pointer (const sal::buf_ptr &) noexcept //{{{1
{
  static T data_[buf_size] = {};
  return &data_[0];
}


template <typename T>
auto *pointer (const sal::const_buf_ptr &) noexcept //{{{1
{
  static const T data_[buf_size] = {};
  return &data_[0];
}


template <typename T>
auto &array (const sal::buf_ptr &) noexcept //{{{1
{
  static T data_[buf_size] = {};
  return data_;
}


template <typename T>
auto &array (const sal::const_buf_ptr &) noexcept //{{{1
{
  static const T data_[buf_size] = {};
  return data_;
}


template <typename T>
auto &std_array (const sal::buf_ptr &) noexcept //{{{1
{
  static std::array<T, buf_size> data_{};
  return data_;
}


template <typename T>
auto &std_array (const sal::const_buf_ptr &) noexcept //{{{1
{
  static const std::array<T, buf_size> data_{};
  return data_;
}


template <typename T>
auto &std_vector (const sal::buf_ptr &) noexcept //{{{1
{
  static std::vector<T> data_(buf_size);
  return data_;
}


template <typename T>
auto &std_vector (const sal::const_buf_ptr &) noexcept //{{{1
{
  static const std::vector<T> data_(buf_size);
  return data_;
}


auto &string (const sal::buf_ptr &) //{{{1
{
  static std::string data_{"test"};
  return data_;
}


auto &string (const sal::const_buf_ptr &) //{{{1
{
  static const std::string data_{"test"};
  return data_;
}


template <typename BufferType>
using buf_ptr = sal_test::with_type<BufferType>;

using buf_ptr_types = testing::Types<
  sal::buf_ptr,
  sal::const_buf_ptr
>;

struct buf_ptr_names
{
  template <typename T>
  static std::string GetName (int i)
  {
    (void)i;
    if constexpr (std::is_same_v<T, sal::buf_ptr>)
    {
      return "mutable";
    }
    else if constexpr (std::is_same_v<T, sal::const_buf_ptr>)
    {
      return "const";
    }
    else
    {
      return std::to_string(i);
    }
  }
};

TYPED_TEST_CASE(buf_ptr, buf_ptr_types, buf_ptr_names);


TYPED_TEST(buf_ptr, ctor) //{{{1
{
  TypeParam ptr;
  EXPECT_EQ(nullptr, ptr.data());
  EXPECT_EQ(0U, ptr.size());
}


TYPED_TEST(buf_ptr, ctor_pointer) //{{{1
{
  TypeParam ptr(pointer<char>(TypeParam()), buf_size);
  EXPECT_EQ(pointer<char>(TypeParam()), ptr.data());
  EXPECT_EQ(buf_size, ptr.size());
}


TYPED_TEST(buf_ptr, inc) //{{{1
{
  TypeParam ptr(pointer<char>(TypeParam()), buf_size);
  ptr += buf_size / 2;
  EXPECT_EQ(pointer<char>(TypeParam()) + 2, ptr.data());
  EXPECT_EQ(buf_size / 2, ptr.size());
}


TYPED_TEST(buf_ptr, inc_invalid) //{{{1
{
  TypeParam ptr(pointer<char>(TypeParam()), buf_size);
  ptr += buf_size * 2;
  EXPECT_EQ(pointer<char>(TypeParam()) + buf_size, ptr.data());
  EXPECT_EQ(0U, ptr.size());
}


TYPED_TEST(buf_ptr, add_ptr_and_size) //{{{1
{
  TypeParam a(pointer<char>(TypeParam()), buf_size);
  TypeParam b = a + 2;
  EXPECT_EQ(pointer<char>(TypeParam()) + 2, b.data());
  EXPECT_EQ(buf_size / 2, b.size());
}


TYPED_TEST(buf_ptr, add_ptr_and_size_invalid) //{{{1
{
  TypeParam a(pointer<char>(TypeParam()), buf_size);
  TypeParam b = a + 2 * buf_size;
  EXPECT_EQ(pointer<char>(TypeParam()) + buf_size, b.data());
  EXPECT_EQ(0U, b.size());
}


TYPED_TEST(buf_ptr, add_size_and_ptr) //{{{1
{
  TypeParam a(pointer<char>(TypeParam()), buf_size);
  TypeParam b = 2 + a;
  EXPECT_EQ(pointer<char>(TypeParam()) + 2, b.data());
  EXPECT_EQ(buf_size / 2, b.size());
}


TYPED_TEST(buf_ptr, add_size_and_ptr_invalid) //{{{1
{
  TypeParam a(pointer<char>(TypeParam()), buf_size);
  TypeParam b = 2 * buf_size + a;
  EXPECT_EQ(pointer<char>(TypeParam()) + buf_size, b.data());
  EXPECT_EQ(0U, b.size());
}


TYPED_TEST(buf_ptr, from_char_pointer) //{{{1
{
  auto *d = pointer<char>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size);
  EXPECT_EQ(d, ptr.data());
  EXPECT_EQ(buf_size, ptr.size());
}


TYPED_TEST(buf_ptr, from_int_pointer) //{{{1
{
  auto *d = pointer<int>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size);
  EXPECT_EQ(d, ptr.data());
  EXPECT_EQ(buf_size, ptr.size());
}


TYPED_TEST(buf_ptr, from_ptr) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam a = sal::make_buf(d);
  TypeParam ptr = sal::make_buf(a);
  EXPECT_EQ(d, ptr.data());
  EXPECT_EQ(buf_size, ptr.size());
}


TYPED_TEST(buf_ptr, from_ptr_half) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam a = sal::make_buf(d);
  TypeParam ptr = sal::make_buf(a, a.size() / 2);
  EXPECT_EQ(d, ptr.data());
  EXPECT_EQ(buf_size / 2, ptr.size());
}


TYPED_TEST(buf_ptr, from_ptr_overflow) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam a = sal::make_buf(d);
  TypeParam ptr = sal::make_buf(a, a.size() * 2);
  EXPECT_EQ(d, ptr.data());
  EXPECT_EQ(buf_size, ptr.size());
}


TYPED_TEST(buf_ptr, from_char_array) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam ptr = sal::make_buf(d);
  EXPECT_EQ(d, ptr.data());
  EXPECT_EQ(buf_size, ptr.size());
}


TYPED_TEST(buf_ptr, from_char_array_half) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size / 2);
  EXPECT_EQ(d, ptr.data());
  EXPECT_EQ(buf_size / 2, ptr.size());
}


TYPED_TEST(buf_ptr, from_char_array_overflow) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size * 1024);
  EXPECT_EQ(d, ptr.data());
  EXPECT_EQ(buf_size, ptr.size());
}


TYPED_TEST(buf_ptr, from_int_array) //{{{1
{
  auto &d = array<int>(TypeParam());
  TypeParam ptr = sal::make_buf(d);
  EXPECT_EQ(d, ptr.data());
  EXPECT_EQ(buf_size * sizeof(int), ptr.size());
}


TYPED_TEST(buf_ptr, from_int_array_half) //{{{1
{
  auto &d = array<int>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size / 2);
  EXPECT_EQ(d, ptr.data());
  EXPECT_EQ(buf_size / 2, ptr.size());
}


TYPED_TEST(buf_ptr, from_int_array_overflow) //{{{1
{
  auto &d = array<int>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size * 1024);
  EXPECT_EQ(d, ptr.data());
  EXPECT_EQ(buf_size * sizeof(int), ptr.size());
}


TYPED_TEST(buf_ptr, from_std_char_array) //{{{1
{
  auto &d = std_array<char>(TypeParam());
  TypeParam ptr = sal::make_buf(d);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(d.size(), ptr.size());
}


TYPED_TEST(buf_ptr, from_std_char_array_half) //{{{1
{
  auto &d = std_array<char>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size / 2);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(buf_size / 2, ptr.size());
}


TYPED_TEST(buf_ptr, from_std_char_array_overflow) //{{{1
{
  auto &d = std_array<char>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size * 1024);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(buf_size, ptr.size());
}


TYPED_TEST(buf_ptr, from_std_int_array) //{{{1
{
  auto &d = std_array<int>(TypeParam());
  TypeParam ptr = sal::make_buf(d);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(d.size() * sizeof(int), ptr.size());
}


TYPED_TEST(buf_ptr, from_std_int_array_half) //{{{1
{
  auto &d = std_array<int>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size / 2);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(buf_size / 2, ptr.size());
}


TYPED_TEST(buf_ptr, from_std_int_array_overflow) //{{{1
{
  auto &d = std_array<int>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size * 1024);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(buf_size * sizeof(int), ptr.size());
}


TYPED_TEST(buf_ptr, from_std_char_vector) //{{{1
{
  auto &d = std_vector<char>(TypeParam());
  TypeParam ptr = sal::make_buf(d);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(d.size(), ptr.size());
}


TYPED_TEST(buf_ptr, from_std_char_vector_half) //{{{1
{
  auto &d = std_vector<char>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size / 2);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(buf_size / 2, ptr.size());
}


TYPED_TEST(buf_ptr, from_std_char_vector_overflow) //{{{1
{
  auto &d = std_vector<char>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size * 1024);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(buf_size, ptr.size());
}


TYPED_TEST(buf_ptr, from_std_int_vector) //{{{1
{
  auto &d = std_vector<int>(TypeParam());
  TypeParam ptr = sal::make_buf(d);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(d.size() * sizeof(int), ptr.size());
}


TYPED_TEST(buf_ptr, from_std_int_vector_half) //{{{1
{
  auto &d = std_vector<int>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size / 2);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(buf_size / 2, ptr.size());
}


TYPED_TEST(buf_ptr, from_std_int_vector_overflow) //{{{1
{
  auto &d = std_vector<int>(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size * 1024);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(buf_size * sizeof(int), ptr.size());
}


TYPED_TEST(buf_ptr, from_string) //{{{1
{
  auto &d = string(TypeParam());
  TypeParam ptr = sal::make_buf(d);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(d.size(), ptr.size());
}


TYPED_TEST(buf_ptr, from_string_half) //{{{1
{
  auto &d = string(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size / 2);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(buf_size / 2, ptr.size());
}


TYPED_TEST(buf_ptr, from_string_overflow) //{{{1
{
  auto &d = string(TypeParam());
  TypeParam ptr = sal::make_buf(d, buf_size * 1024);
  EXPECT_EQ(d.data(), ptr.data());
  EXPECT_EQ(d.size(), ptr.size());
}


TYPED_TEST(buf_ptr, range_loop_string) //{{{1
{
  auto &d = string(TypeParam());
  std::string result;
  for (auto b: d)
  {
    result += b;
  }
  EXPECT_EQ(d, result);
}


TYPED_TEST(buf_ptr, range_loop_std_int_vector) //{{{1
{
  auto &d = std_vector<int>(TypeParam());
  std::vector<int> result;
  for (auto b: d)
  {
    result.emplace_back(b);
  }
  EXPECT_EQ(d, result);
}


} // namespace
