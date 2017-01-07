#include <sal/ptr.hpp>
#include <sal/common.test.hpp>


namespace {


constexpr size_t size = 4;


template <typename T>
auto *pointer (const sal::ptr_t &) noexcept //{{{1
{
  static T data_[size] = {};
  return &data_[0];
}


template <typename T>
auto *pointer (const sal::const_ptr_t &) noexcept //{{{1
{
  static const T data_[size] = {};
  return &data_[0];
}


template <typename T>
auto &array (const sal::ptr_t &) noexcept //{{{1
{
  static T data_[size] = {};
  return data_;
}


template <typename T>
auto &array (const sal::const_ptr_t &) noexcept //{{{1
{
  static const T data_[size] = {};
  return data_;
}


template <typename T>
auto &std_array (const sal::ptr_t &) noexcept //{{{1
{
  static std::array<T, size> data_{};
  return data_;
}


template <typename T>
auto &std_array (const sal::const_ptr_t &) noexcept //{{{1
{
  static const std::array<T, size> data_{};
  return data_;
}


template <typename T>
auto &std_vector (const sal::ptr_t &) noexcept //{{{1
{
  static std::vector<T> data_(size);
  return data_;
}


template <typename T>
auto &std_vector (const sal::const_ptr_t &) noexcept //{{{1
{
  static const std::vector<T> data_(size);
  return data_;
}


auto &string (const sal::ptr_t &) //{{{1
{
  static std::string data_{"test"};
  return data_;
}


auto &string (const sal::const_ptr_t &) //{{{1
{
  static const std::string data_{"test"};
  return data_;
}


template <typename BufferType>
using ptr = sal_test::with_type<BufferType>;


using ptr_types = testing::Types<
  sal::ptr_t,
  sal::const_ptr_t
>;

TYPED_TEST_CASE(ptr, ptr_types);


TYPED_TEST(ptr, ctor) //{{{1
{
  TypeParam ptr;
  EXPECT_EQ(nullptr, ptr.get());
  EXPECT_EQ(0U, ptr.size());
}


TYPED_TEST(ptr, ctor_pointer) //{{{1
{
  TypeParam ptr(pointer<char>(TypeParam()), size);
  EXPECT_EQ(pointer<char>(TypeParam()), ptr.get());
  EXPECT_EQ(size, ptr.size());
}


TYPED_TEST(ptr, inc) //{{{1
{
  TypeParam ptr(pointer<char>(TypeParam()), size);
  ptr += size / 2;
  EXPECT_EQ(pointer<char>(TypeParam()) + 2, ptr.get());
  EXPECT_EQ(size / 2, ptr.size());
}


TYPED_TEST(ptr, inc_invalid) //{{{1
{
  TypeParam ptr(pointer<char>(TypeParam()), size);
  ptr += size * 2;
  EXPECT_EQ(pointer<char>(TypeParam()) + size, ptr.get());
  EXPECT_EQ(0U, ptr.size());
}


TYPED_TEST(ptr, add_ptr_and_size) //{{{1
{
  TypeParam a(pointer<char>(TypeParam()), size);
  TypeParam b = a + 2;
  EXPECT_EQ(pointer<char>(TypeParam()) + 2, b.get());
  EXPECT_EQ(size / 2, b.size());
}


TYPED_TEST(ptr, add_ptr_and_size_invalid) //{{{1
{
  TypeParam a(pointer<char>(TypeParam()), size);
  TypeParam b = a + 2 * size;
  EXPECT_EQ(pointer<char>(TypeParam()) + size, b.get());
  EXPECT_EQ(0U, b.size());
}


TYPED_TEST(ptr, add_size_and_ptr) //{{{1
{
  TypeParam a(pointer<char>(TypeParam()), size);
  TypeParam b = 2 + a;
  EXPECT_EQ(pointer<char>(TypeParam()) + 2, b.get());
  EXPECT_EQ(size / 2, b.size());
}


TYPED_TEST(ptr, add_size_and_ptr_invalid) //{{{1
{
  TypeParam a(pointer<char>(TypeParam()), size);
  TypeParam b = 2 * size + a;
  EXPECT_EQ(pointer<char>(TypeParam()) + size, b.get());
  EXPECT_EQ(0U, b.size());
}


TYPED_TEST(ptr, from_char_pointer) //{{{1
{
  auto *d = pointer<char>(TypeParam());
  TypeParam ptr = sal::ptr(d, size);
  EXPECT_EQ(d, ptr.get());
  EXPECT_EQ(size, ptr.size());
}


TYPED_TEST(ptr, from_int_pointer) //{{{1
{
  auto *d = pointer<int>(TypeParam());
  TypeParam ptr = sal::ptr(d, size);
  EXPECT_EQ(d, ptr.get());
  EXPECT_EQ(size, ptr.size());
}


TYPED_TEST(ptr, from_ptr) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam a = sal::ptr(d);
  TypeParam ptr = sal::ptr(a);
  EXPECT_EQ(d, ptr.get());
  EXPECT_EQ(size, ptr.size());
}


TYPED_TEST(ptr, from_ptr_half) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam a = sal::ptr(d);
  TypeParam ptr = sal::ptr(a, a.size() / 2);
  EXPECT_EQ(d, ptr.get());
  EXPECT_EQ(size / 2, ptr.size());
}


TYPED_TEST(ptr, from_ptr_overflow) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam a = sal::ptr(d);
  TypeParam ptr = sal::ptr(a, a.size() * 2);
  EXPECT_EQ(d, ptr.get());
  EXPECT_EQ(size, ptr.size());
}


TYPED_TEST(ptr, from_char_array) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam ptr = sal::ptr(d);
  EXPECT_EQ(d, ptr.get());
  EXPECT_EQ(size, ptr.size());
}


TYPED_TEST(ptr, from_char_array_half) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam ptr = sal::ptr(d, size / 2);
  EXPECT_EQ(d, ptr.get());
  EXPECT_EQ(size / 2, ptr.size());
}


TYPED_TEST(ptr, from_char_array_overflow) //{{{1
{
  auto &d = array<char>(TypeParam());
  TypeParam ptr = sal::ptr(d, size * 1024);
  EXPECT_EQ(d, ptr.get());
  EXPECT_EQ(size, ptr.size());
}


TYPED_TEST(ptr, from_int_array) //{{{1
{
  auto &d = array<int>(TypeParam());
  TypeParam ptr = sal::ptr(d);
  EXPECT_EQ(d, ptr.get());
  EXPECT_EQ(size * sizeof(int), ptr.size());
}


TYPED_TEST(ptr, from_int_array_half) //{{{1
{
  auto &d = array<int>(TypeParam());
  TypeParam ptr = sal::ptr(d, size / 2);
  EXPECT_EQ(d, ptr.get());
  EXPECT_EQ(size / 2, ptr.size());
}


TYPED_TEST(ptr, from_int_array_overflow) //{{{1
{
  auto &d = array<int>(TypeParam());
  TypeParam ptr = sal::ptr(d, size * 1024);
  EXPECT_EQ(d, ptr.get());
  EXPECT_EQ(size * sizeof(int), ptr.size());
}


TYPED_TEST(ptr, from_std_char_array) //{{{1
{
  auto &d = std_array<char>(TypeParam());
  TypeParam ptr = sal::ptr(d);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(d.size(), ptr.size());
}


TYPED_TEST(ptr, from_std_char_array_half) //{{{1
{
  auto &d = std_array<char>(TypeParam());
  TypeParam ptr = sal::ptr(d, size / 2);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(size / 2, ptr.size());
}


TYPED_TEST(ptr, from_std_char_array_overflow) //{{{1
{
  auto &d = std_array<char>(TypeParam());
  TypeParam ptr = sal::ptr(d, size * 1024);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(size, ptr.size());
}


TYPED_TEST(ptr, from_std_int_array) //{{{1
{
  auto &d = std_array<int>(TypeParam());
  TypeParam ptr = sal::ptr(d);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(d.size() * sizeof(int), ptr.size());
}


TYPED_TEST(ptr, from_std_int_array_half) //{{{1
{
  auto &d = std_array<int>(TypeParam());
  TypeParam ptr = sal::ptr(d, size / 2);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(size / 2, ptr.size());
}


TYPED_TEST(ptr, from_std_int_array_overflow) //{{{1
{
  auto &d = std_array<int>(TypeParam());
  TypeParam ptr = sal::ptr(d, size * 1024);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(size * sizeof(int), ptr.size());
}


TYPED_TEST(ptr, from_std_char_vector) //{{{1
{
  auto &d = std_vector<char>(TypeParam());
  TypeParam ptr = sal::ptr(d);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(d.size(), ptr.size());
}


TYPED_TEST(ptr, from_std_char_vector_half) //{{{1
{
  auto &d = std_vector<char>(TypeParam());
  TypeParam ptr = sal::ptr(d, size / 2);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(size / 2, ptr.size());
}


TYPED_TEST(ptr, from_std_char_vector_overflow) //{{{1
{
  auto &d = std_vector<char>(TypeParam());
  TypeParam ptr = sal::ptr(d, size * 1024);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(size, ptr.size());
}


TYPED_TEST(ptr, from_std_int_vector) //{{{1
{
  auto &d = std_vector<int>(TypeParam());
  TypeParam ptr = sal::ptr(d);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(d.size() * sizeof(int), ptr.size());
}


TYPED_TEST(ptr, from_std_int_vector_half) //{{{1
{
  auto &d = std_vector<int>(TypeParam());
  TypeParam ptr = sal::ptr(d, size / 2);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(size / 2, ptr.size());
}


TYPED_TEST(ptr, from_std_int_vector_overflow) //{{{1
{
  auto &d = std_vector<int>(TypeParam());
  TypeParam ptr = sal::ptr(d, size * 1024);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(size * sizeof(int), ptr.size());
}


TYPED_TEST(ptr, from_string) //{{{1
{
  auto &d = string(TypeParam());
  TypeParam ptr = sal::ptr(d);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(d.size(), ptr.size());
}


TYPED_TEST(ptr, from_string_half) //{{{1
{
  auto &d = string(TypeParam());
  TypeParam ptr = sal::ptr(d, size / 2);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(size / 2, ptr.size());
}


TYPED_TEST(ptr, from_string_overflow) //{{{1
{
  auto &d = string(TypeParam());
  TypeParam ptr = sal::ptr(d, size * 1024);
  EXPECT_EQ(d.data(), ptr.get());
  EXPECT_EQ(d.size(), ptr.size());
}


} // namespace
