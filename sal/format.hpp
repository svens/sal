#pragma once

/**
 * \file sal/format.hpp
 * Memory range formatted content writer.
 *
 * This header provides list of inserter operator specialisations for
 * formatted content adding to sal::memory_writer_t. User defined types can
 * implement their own specialisations as well. Doing it will plug those
 * types into sal::memory_writer_t::print() ecosystem.
 *
 * \see sal/memory_writer.hpp
 */

#include <sal/config.hpp>
#include <sal/__bits/format.hpp>
#include <string>


__sal_begin


/**
 * Insert into \a writer string \c true or \c false depending on \a value
 */
inline memory_writer_t &operator<< (memory_writer_t &writer, bool value)
  noexcept
{
  return __bits::format_bool(writer, value);
}


/**
 * Insert into \a writer string \c (null)
 */
inline memory_writer_t &operator<< (memory_writer_t &writer, std::nullptr_t)
  noexcept
{
  return __bits::format_null(writer, nullptr);
}


/**
 * Insert into \a writer formatted human readable numeric \a value
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned long long value) noexcept
{
  return __bits::format_uint(writer, value);
}


/**
 * Insert into \a writer formatted human readable numeric \a value
 */
inline memory_writer_t &operator<< (memory_writer_t &writer, long long value)
  noexcept
{
  return __bits::format_int(writer, value);
}


/**
 * Insert into \a writer formatted human readable numeric \a value
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned long value) noexcept
{
  return __bits::format_uint(writer, static_cast<unsigned long long>(value));
}


/**
 * Insert into \a writer formatted human readable numeric \a value
 */
inline memory_writer_t &operator<< (memory_writer_t &writer, long value)
  noexcept
{
  return __bits::format_int(writer, static_cast<long long>(value));
}


/**
 * Insert into \a writer formatted human readable numeric \a value
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned int value) noexcept
{
  return __bits::format_uint(writer, static_cast<unsigned long long>(value));
}


/**
 * Insert into \a writer formatted human readable numeric \a value
 */
inline memory_writer_t &operator<< (memory_writer_t &writer, int value)
  noexcept
{
  return __bits::format_int(writer, static_cast<long long>(value));
}


/**
 * Insert into \a writer formatted human readable numeric \a value
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  unsigned short value) noexcept
{
  return __bits::format_uint(writer, static_cast<unsigned long long>(value));
}


/**
 * Insert into \a writer formatted human readable numeric \a value
 */
inline memory_writer_t &operator<< (memory_writer_t &writer, short value)
  noexcept
{
  return __bits::format_int(writer, static_cast<long long>(value));
}


/**
 * Create and return opaque object signalling inserter operator to write
 * numeric \a value hexadecimal string representation into memory_writer_t.
 */
template <typename T>
inline constexpr auto hex (T value) noexcept
{
  return __bits::hex_t<T>(value);
}


/**
 * Create and return opaque object signalling inserter operator to write
 * numeric \a value octal string representation into memory_writer_t.
 */
template <typename T>
inline constexpr auto oct (T value) noexcept
{
  return __bits::oct_t<T>(value);
}


/**
 * Create and return opaque object signalling inserter operator to write
 * numeric \a value binary string representation into memory_writer_t.
 */
template <typename T>
inline constexpr auto bin (T value) noexcept
{
  return __bits::bin_t<T>(value);
}


/**
 * Insert into \a writer formatted human readable hexadecimal representation
 * of \a value
 */
template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const __bits::hex_t<T> &value) noexcept
{
  return __bits::format_base(writer, value);
}


/**
 * Insert into \a writer formatted human readable octal representation of
 * \a value
 */
template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const __bits::oct_t<T> &value) noexcept
{
  return __bits::format_base(writer, value);
}


/**
 * Insert into \a writer formatted human readable binary representation of
 * \a value
 */
template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const __bits::bin_t<T> &value) noexcept
{
  return __bits::format_base(writer, value);
}


/**
 * Insert into \a writer formatted human readable float \a value. It is
 * formatted using \c printf("%g").
 */
inline memory_writer_t &operator<< (memory_writer_t &writer, float value)
  noexcept
{
  return __bits::format_float(writer, "%g", value);
}


/**
 * Insert into \a writer formatted human readable float \a value. It is
 * formatted using \c printf("%g").
 */
inline memory_writer_t &operator<< (memory_writer_t &writer, double value)
  noexcept
{
  return __bits::format_float(writer, "%g", value);
}


/**
 * Insert into \a writer formatted human readable float \a value. It is
 * formatted using \c printf("%Lg").
 */
inline memory_writer_t &operator<< (memory_writer_t &writer, long double value)
  noexcept
{
  return __bits::format_float(writer, "%Lg", value);
}


/**
 * Create and return opaque object signalling inserter operator to write float
 * \a value textual representation into memory_writer_t.
 *
 * \note While faster than directly inserting float values into
 * memory_writer_t, this approach allows only limited range of \a value
 * between negative and positive values of
 * \code
 * std::numeric_limits<unsigned long long>::max() / (10^precision)
 * \endcode
 */
template <typename T>
inline constexpr auto fixed_float (T value, size_t precision=2) noexcept
{
  return __bits::fixed_float_t<T>(value, precision);
}


/**
 * Insert into \a writer formatted human readable float representation of
 * \a value
 */
template <typename T>
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const __bits::fixed_float_t<T> &value) noexcept
{
  return __bits::format_fixed_float(writer, value);
}


/**
 * Insert into \a writer formatted human readable pointer \a value. It is
 * formatted as hexadecimal \a value casted to \c uintptr_t with prefix \c 0x
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const void *value) noexcept
{
  return __bits::format_ptr(writer, value);
}


/**
 * Insert into \a writer content of \a value
 */
inline memory_writer_t &operator<< (memory_writer_t &writer,
  const std::string &value) noexcept
{
  return writer.write(value.data(), value.data() + value.size());
}


__sal_end
