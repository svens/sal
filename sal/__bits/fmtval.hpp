#pragma once

#include <sal/builtins.hpp>
#include <sal/config.hpp>
#include <cstring>
#include <memory>
#include <sstream>
#include <type_traits>

#if defined(_MSC_VER)
  // see make_iterator below
  #include <iterator>
#endif


namespace sal {
__sal_begin


namespace __bits {


// helper to silence MSVC 'Checked Iterators' warning
template <typename It>
constexpr inline auto make_iterator (It *it) noexcept
{
#if defined(_MSC_VER)
  return stdext::make_unchecked_array_iterator(it);
#else
  return it;
#endif
}


// helper: [first,last) -> d_first unless doesn't fit
template <typename InputIt, typename ForwardIt>
inline ForwardIt copy_s (InputIt first, InputIt last,
  ForwardIt d_first, ForwardIt d_last) noexcept
{
  auto end = d_first + (last - first);
  if (end <= d_last)
  {
    std::uninitialized_copy(first, last, make_iterator(d_first));
  }

  return end;
}


// bool
inline char *fmt_v (bool value, char *first, char *last) noexcept
{
  if (value)
  {
    static constexpr char s[] = "true";
    return copy_s(s, s + sizeof(s) - 1, first, last);
  }

  static constexpr char s[] = "false";
  return copy_s(s, s + sizeof(s) - 1, first, last);
}


// char
inline char *fmt_v (char value, char *first, char *last) noexcept
{
  return copy_s(&value, &value + 1, first, last);
}


// signed char
inline char *fmt_v (signed char value, char *first, char *last) noexcept
{
  return fmt_v(static_cast<char>(value), first, last);
}


// unsigned char
inline char *fmt_v (unsigned char value, char *first, char *last) noexcept
{
  return fmt_v(static_cast<char>(value), first, last);
}


// helper:
inline unsigned digit_count (uint64_t value) noexcept
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

  unsigned t = (64 - sal_clz(value | 1)) * 1233 >> 12;
  return t - (value < pow10[t]) + 1;
}


// uint64_t
inline char *fmt_v (uint64_t value, char *first, char *last) noexcept
{
  // https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920

  first += digit_count(value);
  if (first <= last)
  {
    static constexpr char digits[] =
      "0001020304050607080910111213141516171819"
      "2021222324252627282930313233343536373839"
      "4041424344454647484950515253545556575859"
      "6061626364656667686970717273747576777879"
      "8081828384858687888990919293949596979899"
    ;

    last = first;
    while (value > 99)
    {
      const auto i = (value % 100) * 2;
      value /= 100;
      *--last = digits[i + 1];
      *--last = digits[i];
    }

    if (value > 9)
    {
      const auto i = value * 2;
      *--last = digits[i + 1];
      *--last = digits[i];
    }
    else
    {
      *--last = static_cast<char>('0' + value);
    }
  }

  return first;
}


// int64_t
inline char *fmt_v (int64_t value, char *first, char *last) noexcept
{
  if (value > -1)
  {
    return fmt_v(static_cast<uint64_t>(value), first, last);
  }

  auto end = fmt_v(0 - static_cast<uint64_t>(value), first + 1, last);
  if (end <= last)
  {
    *first = '-';
  }

  return end;
}


// uint32_t
inline char *fmt_v (uint32_t value, char *first, char *last) noexcept
{
  return fmt_v(static_cast<uint64_t>(value), first, last);
}


// int32_t
inline char *fmt_v (int32_t value, char *first, char *last) noexcept
{
  return fmt_v(static_cast<int64_t>(value), first, last);
}


// uint16_t
inline char *fmt_v (uint16_t value, char *first, char *last) noexcept
{
  return fmt_v(static_cast<uint64_t>(value), first, last);
}


// int16_t
inline char *fmt_v (int16_t value, char *first, char *last) noexcept
{
  return fmt_v(static_cast<int64_t>(value), first, last);
}


// wrap T into base_cast to signal fmt_v intention
template <typename T, size_t Base>
struct base_cast
{
  static_assert(std::is_integral<T>::value, "expected integral type");

  using type = std::make_unsigned_t<
    std::conditional_t<std::is_same<bool, T>::value, uint8_t, T>
  >;
  type data;

  base_cast (T data)
    : data(data)
  {}
};


template <typename T> using hex = base_cast<T, 16>;
template <typename T> using oct = base_cast<T, 8>;
template <typename T> using bin = base_cast<T, 2>;


template <typename T>
inline char *fmt_v (hex<T> value, char *first, char *last) noexcept
{
  auto v = value.data;
  do
  {
    ++first;
  } while (v >>= 4);

  if (first <= last)
  {
    v = value.data;
    last = first;
    do
    {
      static constexpr char digits[] = "0123456789abcdef";
      *--last = digits[v & 0xf];
    } while (v >>= 4);
  }

  return first;
}


template <typename T>
inline char *fmt_v (oct<T> value, char *first, char *last) noexcept
{
  auto v = value.data;
  do
  {
    ++first;
  } while (v >>= 3);

  if (first <= last)
  {
    v = value.data;
    last = first;
    do
    {
      *--last = (v & 7) + '0';
    } while (v >>= 3);
  }

  return first;
}


template <typename T>
inline char *fmt_v (bin<T> value, char *first, char *last) noexcept
{
  auto v = value.data;
  do
  {
    ++first;
  } while (v >>= 1);

  if (first <= last)
  {
    v = value.data;
    last = first;
    do
    {
      *--last = (v & 1) + '0';
    } while (v >>= 1);
  }

  return first;
}


template <typename T, size_t FmtSize>
inline char *fmt_g (const T &value, const char (&fmt)[FmtSize],
  char *first, char *last) noexcept
{
  constexpr auto max_result_size = 26;
  if (last - first >= max_result_size)
  {
    // happy path, result fits directly into range
    return first + std::snprintf(first, last - first, fmt, value);
  }

  // result might not fit into specified range, go through temporary buffer
  char data[max_result_size + 1];
  auto end = data + std::snprintf(data, sizeof(data), fmt, value);
  return copy_s(data, end, first, last);
}


inline char *fmt_v (float value, char *first, char *last) noexcept
{
  return fmt_g(value, "%g", first, last);
}


inline char *fmt_v (double value, char *first, char *last) noexcept
{
  return fmt_g(value, "%g", first, last);
}


inline char *fmt_v (const long double &value, char *first, char *last)
  noexcept
{
  return fmt_g(value, "%Lg", first, last);
}


// nullptr
inline char *fmt_v (std::nullptr_t /**/, char *first, char *last) noexcept
{
  static constexpr char s[] = "(null)";
  return copy_s(s, s + sizeof(s) - 1, first, last);
}


// const T *
template <typename T>
inline char *fmt_v (const T *value, char *first, char *last) noexcept
{
  auto end = fmt_v(hex<uintptr_t>(reinterpret_cast<uintptr_t>(value)),
    first + 2, last
  );

  if (end <= last)
  {
    first[0] = '0';
    first[1] = 'x';
  }

  return end;
}


// const char *
inline char *fmt_v (const char *value, char *first, char *last) noexcept
{
  return value
    ? copy_s(value, value + std::strlen(value), first, last)
    : fmt_v(nullptr, first, last)
  ;
}


// T *
template <typename T>
inline char *fmt_v (T *value, char *first, char *last) noexcept
{
  return fmt_v(static_cast<const T *>(value), first, last);
}


// std::string
inline char *fmt_v (const std::string &value, char *first, char *last)
  noexcept
{
  return copy_s(value.begin(), value.end(), first, last);
}


// catch-all
template <typename T>
inline char *fmt_v (const T &value, char *first, char *last)
{
  std::ostringstream oss;
  oss << value;
  return fmt_v(oss.str(), first, last);
}


} // namespace __bits


__sal_end
} // namespace sal
