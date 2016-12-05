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


inline char *c_str (char *first, char *last, bool in) noexcept
{
  if (in)
  {
    static constexpr const char label[] = "true";
    return copy(first, last, label);
  }
  static constexpr const char label[] = "false";
  return copy(first, last, label);
}


inline char *c_str (char *first, char *last, std::nullptr_t) noexcept
{
  static constexpr const char label[] = "(null)";
  return copy(first, last, label);
}


inline char *c_str (char *first, char *last, unsigned long long in) noexcept
{
  auto size = __bits::digits(in);

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

    while (in > 99)
    {
      const auto i = (in % 100) * 2;
      in /= 100;
      *--first = digits[i + 1];
      *--first = digits[i];
    }

    if (in > 9)
    {
      const auto i = in * 2;
      *--first = digits[i + 1];
      *--first = digits[i];
    }
    else
    {
      *--first = static_cast<char>('0' + in);
    }
  }
  else
  {
    last = first;
  }

  return last;
}


inline char *c_str (char *first, char *last, long long in) noexcept
{
  if (in > -1)
  {
    return c_str(first, last, static_cast<unsigned long long>(in));
  }

  auto end = c_str(first + 1, last,
    0 - static_cast<unsigned long long>(in)
  );

  if (end < last)
  {
    *first = '-';
  }

  return end;
}


inline char *c_str (char *first, char *last, unsigned long in) noexcept
{
  return c_str(first, last, static_cast<unsigned long long>(in));
}


inline char *c_str (char *first, char *last, long in) noexcept
{
  return c_str(first, last, static_cast<long long>(in));
}


inline char *c_str (char *first, char *last, unsigned int in) noexcept
{
  return c_str(first, last, static_cast<unsigned long long>(in));
}


inline char *c_str (char *first, char *last, int in) noexcept
{
  return c_str(first, last, static_cast<long long>(in));
}


inline char *c_str (char *first, char *last, unsigned short in) noexcept
{
  return c_str(first, last, static_cast<unsigned long long>(in));
}


inline char *c_str (char *first, char *last, short in) noexcept
{
  return c_str(first, last, static_cast<long long>(in));
}


inline char *c_str (char *first, char *last, unsigned char in) noexcept
{
  return c_str(first, last, static_cast<unsigned long long>(in));
}


inline char *c_str (char *first, char *last, char in) noexcept
{
  return c_str(first, last, static_cast<long long>(in));
}


namespace __bits {

template <typename Float, size_t FormatSize>
inline char *fmt_g (char *first, char *last,
  const char (&f)[FormatSize], const Float &in) noexcept
{
  constexpr auto max_result_size = 26U;
  if (last - first >= max_result_size)
  {
    // happy path, result fits directly into [first,last)
    return first + std::snprintf(first, last - first, f, in);
  }

  // might not fit, go through temporary buffer
  char buffer[max_result_size + 1];
  auto size = std::snprintf(buffer, sizeof(buffer), f, in);

  // copy to result (if fits), including NUL and return ptr to end
  return copy(first, last, buffer, buffer + size + 1) - 1;
}

} // namespace __bits


inline char *c_str (char *first, char *last, float in) noexcept
{
  return __bits::fmt_g(first, last, "%g", in);
}


inline char *c_str (char *first, char *last, double in) noexcept
{
  return __bits::fmt_g(first, last, "%g", in);
}


inline char *c_str (char *first, char *last, long double in) noexcept
{
  return __bits::fmt_g(first, last, "%Lg", in);
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
inline constexpr auto hex (T in) noexcept
{
  return __bits::hex_t<T>{in};
}


template <typename T>
inline char *c_str (char *first, char *last, __bits::hex_t<T> in) noexcept
{
  auto data = in.data;
  auto size = 0U;
  do
  {
    ++size;
  } while (data >>= 4);

  first += size;
  if (first < last)
  {
    *(last = first) = '\0';
    data = in.data;
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
inline constexpr auto oct (T in) noexcept
{
  return __bits::oct_t<T>{in};
}


template <typename T>
inline char *c_str (char *first, char *last, __bits::oct_t<T> in) noexcept
{
  auto data = in.data;
  auto size = 0U;
  do
  {
    ++size;
  } while (data >>= 3);

  first += size;
  if (first < last)
  {
    *(last = first) = '\0';
    data = in.data;
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
inline constexpr auto bin (T in) noexcept
{
  return __bits::bin_t<T>{in};
}


template <typename T>
inline char *c_str (char *first, char *last, __bits::bin_t<T> in) noexcept
{
  auto data = in.data;
  auto size = 0U;
  do
  {
    ++size;
  } while (data >>= 1);

  first += size;
  if (first < last)
  {
    *(last = first) = '\0';
    data = in.data;
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
