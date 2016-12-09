#pragma once

/**
 * \file sal/format.hpp
 * Memory range formatted content writer
 */

#include <sal/config.hpp>
#include <sal/__bits/format.hpp>
#include <string>


__sal_begin


inline memory_writer_t &operator<< (memory_writer_t &writer, bool value)
  noexcept
{
  return __bits::format_bool(writer, value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, std::nullptr_t)
  noexcept
{
  return __bits::format_null(writer, nullptr);
}


inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned long long value) noexcept
{
  return __bits::format_uint(writer, value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, long long value)
  noexcept
{
  return __bits::format_int(writer, value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned long value) noexcept
{
  return __bits::format_uint(writer, static_cast<unsigned long long>(value));
}


inline memory_writer_t &operator<< (memory_writer_t &writer, long value)
  noexcept
{
  return __bits::format_int(writer, static_cast<long long>(value));
}


inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned int value) noexcept
{
  return __bits::format_uint(writer, static_cast<unsigned long long>(value));
}


inline memory_writer_t &operator<< (memory_writer_t &writer, int value)
  noexcept
{
  return __bits::format_int(writer, static_cast<long long>(value));
}


inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned short value) noexcept
{
  return __bits::format_uint(writer, static_cast<unsigned long long>(value));
}


inline memory_writer_t &operator<< (memory_writer_t &writer, short value)
  noexcept
{
  return __bits::format_int(writer, static_cast<long long>(value));
}


template <typename T>
inline constexpr auto hex (T value) noexcept
{
  return __bits::hex_t<T>(value);
}


template <typename T>
inline constexpr auto oct (T value) noexcept
{
  return __bits::oct_t<T>(value);
}


template <typename T>
inline constexpr auto bin (T value) noexcept
{
  return __bits::bin_t<T>(value);
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const __bits::hex_t<T> &value) noexcept
{
  return __bits::format_base(writer, value);
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const __bits::oct_t<T> &value) noexcept
{
  return __bits::format_base(writer, value);
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const __bits::bin_t<T> &value) noexcept
{
  return __bits::format_base(writer, value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, float value)
  noexcept
{
  return __bits::format_float(writer, "%g", value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, double value)
  noexcept
{
  return __bits::format_float(writer, "%g", value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer, long double value)
  noexcept
{
  return __bits::format_float(writer, "%Lg", value);
}


template <typename T>
inline constexpr auto fixed_float (T value, size_t precision=2) noexcept
{
  return __bits::fixed_float_t<T>(value, precision);
}


template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const __bits::fixed_float_t<T> &value) noexcept
{
  return __bits::format_fixed_float(writer, value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer,
  const void *value) noexcept
{
  return __bits::format_ptr(writer, value);
}


inline memory_writer_t &operator<< (memory_writer_t &writer,
  const std::string &value) noexcept
{
  return writer.write(value.data(), value.data() + value.size());
}


__sal_end
