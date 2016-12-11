#pragma once


// DO NOT INCLUDE DIRECTLY
// included by sal/format.hpp


#include <sal/builtins.hpp>
#include <sal/memory_writer.hpp>
#include <cmath>
#include <cstdio>
#include <type_traits>


__sal_begin


namespace __bits {


inline memory_writer_t &format_bool (memory_writer_t &writer, bool value)
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


inline memory_writer_t &format_null (memory_writer_t &writer, std::nullptr_t)
  noexcept
{
  static constexpr const char label[] = "(null)";
  return writer.write(label, label + sizeof(label) - 1);
}


inline memory_writer_t &format_uint (memory_writer_t &writer,
  unsigned long long value) noexcept
{
  writer.first += digit_count(value);
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


inline memory_writer_t &format_int (memory_writer_t &writer, long long value)
  noexcept
{
  if (value > -1)
  {
    return format_uint(writer, static_cast<unsigned long long>(value));
  }

  auto first = writer.first;
  if (format_uint(writer.skip(1), 0 - static_cast<unsigned long long>(value)))
  {
    *first = '-';
  }

  return writer;
}


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


template <typename T>
inline memory_writer_t &format_base (memory_writer_t &writer,
  const __bits::hex_t<T> &value) noexcept
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
inline memory_writer_t &format_base (memory_writer_t &writer,
  const __bits::oct_t<T> &value) noexcept
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
inline memory_writer_t &format_base (memory_writer_t &writer,
  const __bits::bin_t<T> &value) noexcept
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


template <typename T, size_t FormatSize>
inline memory_writer_t &format_float (memory_writer_t &writer,
  const char (&format)[FormatSize],
  const T &value) noexcept
{
  constexpr auto max_result_size = 26U;
  auto length = writer.second - writer.first;
  if (length > max_result_size)
  {
    // happy path, result will fit directly to buffer
    writer.first += std::snprintf(writer.first, length, format, value);
  }
  else
  {
    // might not fit, go through temporary buffer
    char buffer[max_result_size + 1];
    length = std::snprintf(buffer, sizeof(buffer), format, value);
    writer.write(buffer, buffer + length);
  }
  return writer;
}


struct basic_fixed_float_t
{
  size_t pow10 (size_t precision) const noexcept
  {
    static constexpr size_t data_[] =
    {
      1,
      10,
      100,
      1000,
      10000,
      100000,
      1000000,
    };
    return data_[precision];
  }
};


template <typename T>
struct fixed_float_t
  : public basic_fixed_float_t
{
  static_assert(std::is_floating_point<T>::value,
    "expected floating point type"
  );

  T data;
  size_t precision;

  constexpr explicit fixed_float_t (T data, size_t precision=2) noexcept
    : data(data)
    , precision(precision < 7 ? precision : 6)
  {}

  memory_writer_t &format (memory_writer_t &writer) const noexcept
  {
    auto value = data < 0 ? (writer.write('-'), -data) : data;
    auto e = pow10(precision);

    auto v = static_cast<unsigned long long>(value * e + 0.5);
    format_uint(writer, v/e).write('.');

    v %= e;
    for (auto p = precision - 1;  p && v < pow10(p);  --p)
    {
      writer.write('0');
    }
    return format_uint(writer, v);
  }
};


template <typename T>
inline memory_writer_t &format_fixed_float (memory_writer_t &writer,
  const fixed_float_t<T> &value) noexcept
{
  if (std::isfinite(value.data))
  {
    return value.format(writer);
  }
  else if (std::isinf(value.data))
  {
    if (value.data < T{})
    {
      writer.write('-');
    }
    static constexpr const char label[] = "inf";
    return writer.write(label, label + sizeof(label) - 1);
  }
  static constexpr const char label[] = "nan";
  return writer.write(label, label + sizeof(label) - 1);
}


inline memory_writer_t &format_ptr (memory_writer_t &writer,
  const void *value) noexcept
{
  auto p = writer.first;
  hex_t<uintptr_t> ptr{reinterpret_cast<uintptr_t>(value)};
  if (format_base(writer.skip(2), ptr))
  {
    *p++ = '0';
    *p = 'x';
  }
  return writer;
}


} // namespace __bits


__sal_end
