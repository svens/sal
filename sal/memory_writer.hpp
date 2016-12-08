#pragma once

/**
 * \file sal/memory_writer.hpp
 * Memory range unformatted and formatted content writer
 */

#include <sal/config.hpp>
#include <sal/builtins.hpp>
#include <cstdio>
#include <cstring>
#include <utility>


__sal_begin


class memory_writer_t
  : public std::pair<char *, const char *>
{
public:

  memory_writer_t (const memory_writer_t &) = delete;
  memory_writer_t &operator= (const memory_writer_t &) = delete;


  template <typename T>
  constexpr memory_writer_t (T *begin, const T *end) noexcept
    : pair{char_p(begin), char_p(end)}
  {}


  template <typename T, size_t N>
  constexpr memory_writer_t (T (&array)[N]) noexcept
    : memory_writer_t{array, array + N}
  {}


  memory_writer_t (memory_writer_t &&w) noexcept
    : pair{std::move(w)}
  {}


  memory_writer_t &operator= (memory_writer_t &&w) noexcept
  {
    pair::operator=(std::move(w));
    return *this;
  }


  void swap (memory_writer_t &that) noexcept
  {
    pair::swap(that);
  }


  constexpr bool good () const noexcept
  {
    return first <= second;
  }


  constexpr explicit operator bool () const noexcept
  {
    return good();
  }


  constexpr bool bad () const noexcept
  {
    return first > second;
  }


  constexpr bool full () const noexcept
  {
    return first == second;
  }


  constexpr size_t size () const noexcept
  {
    return second - first;
  }


  constexpr char *begin () const noexcept
  {
    return first;
  }


  constexpr const char *end () const noexcept
  {
    return second;
  }


  memory_writer_t &skip (size_t n) noexcept
  {
    first += n;
    return *this;
  }


  template <typename T>
  memory_writer_t &write (T v) noexcept
  {
    if (first + sizeof(v) <= second)
    {
      *reinterpret_cast<T *>(first) = v;
    }
    first += sizeof(v);
    return *this;
  }


  template <typename T>
  memory_writer_t &write (const T *begin, const T *end) noexcept
  {
    auto size = char_p(end) - char_p(begin);
    if (first + size <= second)
    {
      std::memcpy(first, char_p(begin), size);
    }
    first += size;
    return *this;
  }


  template <typename T, size_t N>
  memory_writer_t &write (const T (&array)[N]) noexcept
  {
    return write(array, array + N);
  }


  template <typename Arg, typename... Args>
  memory_writer_t &print (Arg &&arg, Args &&...args) noexcept
  {
    bool unused[] = { (*this << arg, false), (*this << args, false)... };
    (void)unused;
    return *this;
  }


private:

  template <typename T>
  constexpr char *char_p (T *p) const noexcept
  {
    static_assert(std::is_pod<T>::value, "expected POD type");
    return reinterpret_cast<char *>(p);
  }

  template <typename T>
  constexpr const char *char_p (const T *p) const noexcept
  {
    static_assert(std::is_pod<T>::value, "expected POD type");
    return reinterpret_cast<const char *>(p);
  }
};


inline memory_writer_t &operator<< (memory_writer_t &writer, bool value)
  noexcept
{
  if (value)
  {
    static constexpr const char label[] = "true";
    return writer.write(label, label + sizeof(label) - 1);
  }
  static constexpr const char label[] = "false";
  return writer.write(label, label + sizeof(label) - 1);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, std::nullptr_t)
  noexcept
{
  static constexpr const char label[] = "(null)";
  return writer.write(label, label + sizeof(label) - 1);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, const char *s)
  noexcept
{
  while (*s && writer.first < writer.second)
  {
    *writer.first++ = *s++;
  }
  while (*s)
  {
    ++s, ++writer.first;
  }
  return writer;
}


inline memory_writer_t &operator<< (memory_writer_t &writer, char value)
  noexcept
{
  if (writer.first < writer.second)
  {
    *writer.first = value;
  }
  ++writer.first;
  return writer;
}


inline memory_writer_t &operator<< (memory_writer_t &writer, signed char value)
  noexcept
{
  return writer << static_cast<char>(value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, unsigned char value)
  noexcept
{
  return writer << static_cast<char>(value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned long long value) noexcept
{
  writer.first += __bits::digits(value);
  if (writer.first <= writer.second)
  {
    auto *p = writer.first;

    static constexpr char digits[] =
      "0001020304050607080910111213141516171819"
      "2021222324252627282930313233343536373839"
      "4041424344454647484950515253545556575859"
      "6061626364656667686970717273747576777879"
      "8081828384858687888990919293949596979899"
    ;

    while (value > 99)
    {
      auto i = (value % 100) * 2;
      value /= 100;
      *--p = digits[i + 1];
      *--p = digits[i];
    }

    if (value > 9)
    {
      auto i = value * 2;
      *--p = digits[i + 1];
      *--p = digits[i];
    }
    else
    {
      *--p = static_cast<char>('0' + value);
    }
  }
  return writer;
}


inline memory_writer_t &operator<< (memory_writer_t &writer, long long value)
  noexcept
{
  if (value > -1)
  {
    return writer << static_cast<unsigned long long>(value);
  }

  auto first = writer.first;
  if (writer.skip(1) << 0 - static_cast<unsigned long long>(value))
  {
    *first = '-';
  }

  return writer;
}


inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned long value) noexcept
{
  return writer << static_cast<unsigned long long>(value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, long value)
  noexcept
{
  return writer << static_cast<long long>(value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned int value) noexcept
{
  return writer << static_cast<unsigned long long>(value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, int value)
  noexcept
{
  return writer << static_cast<long long>(value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned short value) noexcept
{
  return writer << static_cast<unsigned long long>(value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, short value)
  noexcept
{
  return writer << static_cast<long long>(value);
}


namespace __bits {

template <typename Float, size_t FormatSize>
inline memory_writer_t &fmt_g (memory_writer_t &writer,
  const char (&format)[FormatSize],
  const Float &value) noexcept
{
  constexpr auto max_result_size = 26U;
  if (writer.size() > max_result_size)
  {
    // happy path, result will fit directly to buffer
    writer.first += std::snprintf(writer.first, writer.size(), format, value);
  }
  else
  {
    // might not fit, go through temporary buffer
    char buffer[max_result_size + 1];
    auto size = std::snprintf(buffer, sizeof(buffer), format, value);
    writer.write(buffer, buffer + size);
  }
  return writer;
}

} // namespace __bits


inline memory_writer_t &operator<< (memory_writer_t &writer, float value)
  noexcept
{
  return __bits::fmt_g(writer, "%g", value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, double value)
  noexcept
{
  return __bits::fmt_g(writer, "%g", value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, long double value)
  noexcept
{
  return __bits::fmt_g(writer, "%Lg", value);
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
  return __bits::hex_t<T>(value);
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &writer,
  __bits::hex_t<T> value) noexcept
{
  auto data = value.data;
  do
  {
    ++writer.first;
  } while (data >>= 4);

  if (writer.first <= writer.second)
  {
    data = value.data;
    auto p = writer.first;
    do
    {
      static constexpr char digits[] = "0123456789abcdef";
      *--p = digits[data & 0xf];
    } while (data >>= 4);
  }

  return writer;
}


template <typename T>
inline constexpr auto oct (T value) noexcept
{
  return __bits::oct_t<T>(value);
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &writer,
  __bits::oct_t<T> value) noexcept
{
  auto data = value.data;
  do
  {
    ++writer.first;
  } while (data >>= 3);

  if (writer.first <= writer.second)
  {
    data = value.data;
    auto p = writer.first;
    do
    {
      *--p = (data & 7) + '0';
    } while (data >>= 3);
  }

  return writer;
}


template <typename T>
inline constexpr auto bin (T value) noexcept
{
  return __bits::bin_t<T>(value);
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &writer,
  __bits::bin_t<T> value) noexcept
{
  auto data = value.data;
  do
  {
    ++writer.first;
  } while (data >>= 1);

  if (writer.first <= writer.second)
  {
    data = value.data;
    auto p = writer.first;
    do
    {
      *--p = (data & 1) + '0';
    } while (data >>= 1);
  }

  return writer;
}


inline memory_writer_t &operator<< (memory_writer_t &writer,
  const void *value) noexcept
{
  auto p = writer.first;
  if (writer.skip(2) << hex(reinterpret_cast<uintptr_t>(value)))
  {
    *p++ = '0';
    *p = 'x';
  }
  return writer;
}


__sal_end
