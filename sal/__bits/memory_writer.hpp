#pragma once


// DO NOT INCLUDE DIRECTLY
// It is included by sal/memory_writer.hpp

#include <sal/builtins.hpp>
#include <cstdio>
#include <memory>

#if defined(_MSC_VER)
  // see make_iterator()
  #include <iterator>
#endif


__sal_begin


namespace __bits {


inline unsigned TODO_digit_count (uint64_t v) noexcept
{
  // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10

  static constexpr uint64_t pow10[] =
  {
    0ULL,
    10ULL,
    100ULL,
    1000ULL,
    10000ULL,
    100000ULL,
    1000000ULL,
    10000000ULL,
    100000000ULL,
    1000000000ULL,
    10000000000ULL,
    100000000000ULL,
    1000000000000ULL,
    10000000000000ULL,
    100000000000000ULL,
    1000000000000000ULL,
    10000000000000000ULL,
    100000000000000000ULL,
    1000000000000000000ULL,
    10000000000000000000ULL,
  };

  unsigned t = (64 - sal_clz(v | 1)) * 1233 >> 12;
  return t - (v < pow10[t]) + 1;
}


inline unsigned fmt (unsigned long long v, char *p, const char * const end)
  noexcept
{
  // https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920

  const auto size = TODO_digit_count(v);
  p += size;
  if (p <= end)
  {
    static constexpr char digits[] =
      "0001020304050607080910111213141516171819"
      "2021222324252627282930313233343536373839"
      "4041424344454647484950515253545556575859"
      "6061626364656667686970717273747576777879"
      "8081828384858687888990919293949596979899"
    ;

    while (v > 99)
    {
      const auto i = (v % 100) * 2;
      v /= 100;
      *--p = digits[i + 1];
      *--p = digits[i];
    }

    if (v > 9)
    {
      const auto i = v * 2;
      *--p = digits[i + 1];
      *--p = digits[i];
    }
    else
    {
      *--p = static_cast<char>('0' + v);
    }
  }
  return size;
}


inline unsigned fmt (long long v, char *p, const char * const end) noexcept
{
  if (v > -1)
  {
    return fmt(static_cast<unsigned long long>(v), p, end);
  }

  auto size = fmt(0 - static_cast<unsigned long long>(v), p + 1, end) + 1;
  if (p + size <= end)
  {
    *p = '-';
  }
  return size;
}


// helper to silence MSVC 'Checked Iterators' warning
template <typename It>
constexpr inline auto TODO_make_iterator (It *it) noexcept
{
#if defined(_MSC_VER)
  return stdext::make_unchecked_array_iterator(it);
#else
  return it;
#endif
}


template <typename Float, size_t FormatSize>
inline unsigned fmt (const Float &v, const char (&f)[FormatSize],
  char *p, const char * const end) noexcept
{
  constexpr auto max_result_size = 26;
  if (end - p >= max_result_size)
  {
    // happy path, result fits directly into range
    return std::snprintf(p, end - p, f, v);
  }

  // result might not fit into specified range, go through temporary buffer
  char buffer[max_result_size + 1];
  auto size = std::snprintf(buffer, sizeof(buffer), f, v);
  if (p + size <= end)
  {
    std::uninitialized_copy(p, const_cast<char *>(end),
      __bits::TODO_make_iterator(buffer)
    );
  }
  return size;
}


template <typename T, size_t Base>
struct base_cast_t
{
  static_assert(std::is_integral<T>::value, "expected integral type");

  using type = std::make_unsigned_t<
    std::conditional_t<std::is_same<bool, T>::value, uint8_t, T>
  >;
  type data;

  explicit base_cast_t (T data) noexcept
    : data{static_cast<type>(data)}
  {}
};


template <typename T> using hex_t = base_cast_t<T, 16>;
template <typename T> using oct_t = base_cast_t<T, 8>;
template <typename T> using bin_t = base_cast_t<T, 2>;


template <typename T>
inline unsigned fmt (hex_t<T> v, char *p, const char * const end) noexcept
{
  auto d = v.data;
  auto size = 0U;
  do
  {
    ++size;
  } while (d >>= 4);

  p += size;
  if (p <= end)
  {
    d = v.data;
    do
    {
      static constexpr char digits[] = "0123456789abcdef";
      *--p = digits[d & 0xf];
    } while (d >>= 4);
  }

  return size;
}


template <typename T>
inline unsigned fmt (oct_t<T> v, char *p, const char * const end) noexcept
{
  auto d = v.data;
  auto size = 0U;
  do
  {
    ++size;
  } while (d >>= 3);

  p += size;
  if (p <= end)
  {
    d = v.data;
    do
    {
      *--p = (d & 7) + '0';
    } while (d >>= 3);
  }

  return size;
}


template <typename T>
inline unsigned fmt (bin_t<T> v, char *p, const char * const end) noexcept
{
  auto d = v.data;
  auto size = 0U;
  do
  {
    ++size;
  } while (d >>= 1);

  p += size;
  if (p <= end)
  {
    d = v.data;
    do
    {
      *--p = (d & 1) + '0';
    } while (d >>= 1);
  }

  return size;
}


inline unsigned fmt (const void *v, char *p, const char *end) noexcept
{
  auto size = fmt(hex_t<uintptr_t>(reinterpret_cast<uintptr_t>(v)), p + 2, end) + 2;
  if (p + size <= end)
  {
    *p++ = '0';
    *p = 'x';
  }
  return size;
}


} // namespace __bits


__sal_end
