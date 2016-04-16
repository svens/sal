#pragma once

/**
 * \file sal/fmtval.hpp
 * Functions to format value to textual representation.
 */


#include <sal/builtins.hpp>
#include <sal/config.hpp>
#include <cstring>
#include <sstream>
#include <type_traits>

__sal_warn_disable
#include <memory>
__sal_warn_enable


namespace sal {
__sal_hpp_begin


namespace __bits {


// helper: [first,last) -> d_first unless doesn't fit
template <typename InputIt, typename ForwardIt>
inline ForwardIt copy_s (InputIt first, InputIt last,
  ForwardIt d_first, ForwardIt d_last) noexcept
{
  auto end = d_first + (last - first);
  if (end <= d_last)
  {
    return std::uninitialized_copy(first, last, d_first);
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

  base_cast (T data):
    data(data)
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


// float/double/long double
char *fmt_v (float value, char *first, char *last) noexcept;
char *fmt_v (double value, char *first, char *last) noexcept;
char *fmt_v (const long double &value, char *first, char *last) noexcept;


// catch-all
template <typename T>
inline char *fmt_v (const T &value, char *first, char *last)
{
  std::ostringstream oss;
  oss << value;
  return fmt_v(oss.str(), first, last);
}


} // namespace __bits


/**
 * Copy \a value human-readable representation to [\a first, \a last).
 *
 * \note Result is not NUL-terminated.
 *
 * - If result fits to specified range, whole text is copied and pointer to
 *   one position past last character written is returned. If caller needs
 *   NUL-terminated string, this is position where NUL should be written.
 * - If \a value representation would not fit into given range, no partial
 *   text will be copied. Function returns same pointer as on success.
 *
 * To check if fmt_v() copied text successfully, check:
 * \code
 * auto end = sal::fmt_v(value, first, last);
 * if (end <= last)
 * {
 *   // success
 *   std::cout << std::string(first, end) << std::endl;
 * }
 * else
 * {
 *   // overflow, need (end - last) bytes
 * }
 * \endcode
 *
 * Library provides specialization for builtin types (integral types, floats,
 * NUL-terminated string, pointers) plus std::string. For other types, it uses
 * operator<<(std::ostream) to get \a value textual representation. If
 * application-specified type has more optimized means to generate textual
 * representation, it can specialize fmt_v() for given type. This way
 * application types plug into SAL logging etc.
 */
template <typename T>
inline char *fmt_v (const T &value, char *first, char *last)
  noexcept(noexcept(__bits::fmt_v(value, first, last)))
{
  return __bits::fmt_v(value, first, last);
}


/**
 * Convenience method wrapping \a dest[\a dest_size] ->
 * [\a dest, \a dest + \a dest_size)
 *
 * \see fmt_v()
 */
template <typename T, size_t dest_size>
inline char *fmt_v (const T &value, char (&dest)[dest_size])
  noexcept(noexcept(__bits::fmt_v(value, dest, dest + dest_size)))
{
  return __bits::fmt_v(value, dest, dest + dest_size);
}


/**
 * Optimized specialization for copying compile-time const \a src[\a src_size]
 * to [\a first, \a last).
 *
 * \see fmt_v()
 */
template <size_t src_size>
inline char *fmt_v (const char (&src)[src_size], char *first, char *last)
  noexcept(noexcept(__bits::copy_s(src, src + src_size - 1, first, last)))
{
  return __bits::copy_s(src, src + src_size - 1, first, last);
}


/**
 * Optimized specialization for copying compile-time const \a src[\a src_size]
 * to \a dest[\a dest_size].
 *
 * \see fmt_v()
 */
template <size_t src_size, size_t dest_size>
inline char *fmt_v (const char (&src)[src_size], char (&dest)[dest_size])
  noexcept(noexcept(__bits::copy_s(src, src + src_size - 1, dest, dest + dest_size)))
{
  static_assert(src_size - 1 <= dest_size, "not enough room");
  return __bits::copy_s(src, src + src_size - 1, dest, dest + dest_size);
}


/**
 * View manipulator to copy \a value as hexadecimal human readable
 * representation. Only integral types are valid for \a T.
 *
 * Usage:
 * \code
 * auto end = sal::fmt_v(sal::hex(42ULL), first, last);
 * \endcode
 *
 * \return Opaque type, do not touch it's internals
 */
template <typename T>
inline __bits::hex<T> hex (T value) noexcept
{
  return __bits::hex<T>{value};
}


/**
 * View manipulator to copy \a value as octal human readable
 * representation. Only integral types are valid for \a T.
 *
 * Usage:
 * \code
 * auto end = sal::fmt_v(sal::oct(42ULL), first, last);
 * \endcode
 *
 * \return Opaque type, do not touch it's internals
 */
template <typename T>
inline __bits::oct<T> oct (T value) noexcept
{
  return __bits::oct<T>{value};
}


/**
 * View manipulator to copy \a value as binary human readable
 * representation. Only integral types are valid for \a T.
 *
 * Usage:
 * \code
 * auto end = sal::fmt_v(sal::bin(42ULL), first, last);
 * \endcode
 *
 * \return Opaque type, do not touch it's internals
 */
template <typename T>
inline __bits::bin<T> bin (T value) noexcept
{
  return __bits::bin<T>{value};
}


__sal_hpp_end
} // namespace sal
