#pragma once

/**
 * \file sal/utility.hpp
 */

#include <sal/config.hpp>
#include <sal/builtins.hpp>
#include <cstdio>
#include <cstring>
#include <type_traits>


__sal_begin


inline char *copy (char *first, char *last,
  const char *ifirst, const char *ilast) noexcept
{
  auto size = ilast - ifirst;
  if (first + size <= last)
  {
    std::memcpy(first, ifirst, size);
  }
  return first + size;
}


template <size_t N>
inline char *copy (char *first, char *last, const char (&in)[N]) noexcept
{
  return copy(first, last, in, in + N);
}


inline char *to_chars (char *first, char *last, bool value) noexcept
{
  if (value)
  {
    static constexpr const char label[] = "true";
    return copy(first, last, label);
  }
  static constexpr const char label[] = "false";
  return copy(first, last, label);
}


inline char *to_chars (char *first, char *last, std::nullptr_t) noexcept
{
  static constexpr const char label[] = "(null)";
  return copy(first, last, label);
}


inline char *to_chars (char *first, char *last, unsigned long long value)
  noexcept
{
  auto size = __bits::digits(value);

  first += size;
  if (first < last)
  {
    *(last = first) = '\0';

    static constexpr char digits[] =
      "0001020304050607080910111213141516171819"
      "2021222324252627282930313233343536373839"
      "4041424344454647484950515253545556575859"
      "6061626364656667686970717273747576777879"
      "8081828384858687888990919293949596979899"
    ;

    while (value > 99)
    {
      const auto i = (value % 100) * 2;
      value /= 100;
      *--first = digits[i + 1];
      *--first = digits[i];
    }

    if (value > 9)
    {
      const auto i = value * 2;
      *--first = digits[i + 1];
      *--first = digits[i];
    }
    else
    {
      *--first = static_cast<char>('0' + value);
    }
  }
  else
  {
    last = first;
  }

  return last;
}


inline char *to_chars (char *first, char *last, long long value) noexcept
{
  if (value > -1)
  {
    return to_chars(first, last, static_cast<unsigned long long>(value));
  }

  auto end = to_chars(first + 1, last,
    0 - static_cast<unsigned long long>(value)
  );

  if (end < last)
  {
    *first = '-';
  }

  return end;
}


inline char *to_chars (char *first, char *last, unsigned long value) noexcept
{
  return to_chars(first, last, static_cast<unsigned long long>(value));
}


inline char *to_chars (char *first, char *last, long value) noexcept
{
  return to_chars(first, last, static_cast<long long>(value));
}


inline char *to_chars (char *first, char *last, unsigned int value) noexcept
{
  return to_chars(first, last, static_cast<unsigned long long>(value));
}


inline char *to_chars (char *first, char *last, int value) noexcept
{
  return to_chars(first, last, static_cast<long long>(value));
}



inline char *to_chars (char *first, char *last, unsigned short value) noexcept
{
  return to_chars(first, last, static_cast<unsigned long long>(value));
}


inline char *to_chars (char *first, char *last, short value) noexcept
{
  return to_chars(first, last, static_cast<long long>(value));
}


inline char *to_chars (char *first, char *last, unsigned char value) noexcept
{
  return to_chars(first, last, static_cast<unsigned long long>(value));
}


inline char *to_chars (char *first, char *last, char value) noexcept
{
  return to_chars(first, last, static_cast<long long>(value));
}


namespace __bits {

template <typename Float, size_t FormatSize>
inline char *fmt_g (char *first, char *last,
  const char (&f)[FormatSize], const Float &value) noexcept
{
  constexpr auto max_result_size = 26U;
  if (last - first >= max_result_size)
  {
    // happy path, result fits directly into [first,last)
    return first + std::snprintf(first, last - first, f, value);
  }

  // might not fit, go through temporary buffer
  char buffer[max_result_size + 1];
  auto size = std::snprintf(buffer, sizeof(buffer), f, value);

  // copy to result (if fits), including NUL and return ptr to end
  return copy(first, last, buffer, buffer + size + 1) - 1;
}

} // namespace __bits


inline char *to_chars (char *first, char *last, float value) noexcept
{
  return __bits::fmt_g(first, last, "%g", value);
}


inline char *to_chars (char *first, char *last, double value) noexcept
{
  return __bits::fmt_g(first, last, "%g", value);
}


inline char *to_chars (char *first, char *last, long double value) noexcept
{
  return __bits::fmt_g(first, last, "%Lg", value);
}


namespace __bits {

template <typename T, size_t>
struct int_base_t
{
  static_assert(std::is_integral<T>::value, "expected integral type");
  using type = std::make_unsigned_t<
    std::conditional_t<std::is_same<bool, T>::value, uint8_t, T>
  >;
  type data;

  constexpr explicit int_base_t (T data) noexcept
    : data(static_cast<type>(data))
  {}
};

template <typename T> using hex_t = int_base_t<T, 16>;
template <typename T> using oct_t = int_base_t<T, 8>;
template <typename T> using bin_t = int_base_t<T, 2>;

} // namespace __bits


template <typename T>
inline constexpr auto hex (T value) noexcept
{
  return __bits::hex_t<T>{value};
}


template <typename T>
inline char *to_chars (char *first, char *last, __bits::hex_t<T> value)
  noexcept
{
  auto data = value.data;
  auto size = 0U;
  do
  {
    ++size;
  } while (data >>= 4);

  first += size;
  if (first < last)
  {
    *(last = first) = '\0';
    data = value.data;
    do
    {
      static constexpr char digits[] = "0123456789abcdef";
      *--first = digits[data & 0xf];
    } while (data >>= 4);
  }
  else
  {
    last = first;
  }

  return last;
}


template <typename T>
inline constexpr auto oct (T value) noexcept
{
  return __bits::oct_t<T>{value};
}


template <typename T>
inline char *to_chars (char *first, char *last, __bits::oct_t<T> value)
  noexcept
{
  auto data = value.data;
  auto size = 0U;
  do
  {
    ++size;
  } while (data >>= 3);

  first += size;
  if (first < last)
  {
    *(last = first) = '\0';
    data = value.data;
    do
    {
      *--first = (data & 7) + '0';
    } while (data >>= 3);
  }
  else
  {
    last = first;
  }

  return last;
}


template <typename T>
inline constexpr auto bin (T value) noexcept
{
  return __bits::bin_t<T>{value};
}


template <typename T>
inline char *to_chars (char *first, char *last, __bits::bin_t<T> value)
  noexcept
{
  auto data = value.data;
  auto size = 0U;
  do
  {
    ++size;
  } while (data >>= 1);

  first += size;
  if (first < last)
  {
    *(last = first) = '\0';
    data = value.data;
    do
    {
      *--first = (data & 1) + '0';
    } while (data >>= 1);
  }
  else
  {
    last = first;
  }

  return last;
}


__sal_end
