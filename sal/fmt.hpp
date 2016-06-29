#pragma once

/**
 * \file sal/fmt.hpp
 * Functions to format value to textual representation.
 */


#include <sal/config.hpp>
#include <sal/__bits/fmt.hpp>


namespace sal {
__sal_begin


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
 * To check if fmt() copied text successfully, check:
 * \code
 * auto end = sal::fmt(value, first, last);
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
 * representation, it can specialize fmt() for given type. This way
 * application types plug into SAL logging etc.
 */
template <typename T>
inline char *fmt (const T &value, char *first, char *last)
  noexcept(noexcept(__bits::fmt(value, first, last)))
{
  return __bits::fmt(value, first, last);
}


/**
 * Convenience method wrapping \a dest[\a dest_size] ->
 * [\a dest, \a dest + \a dest_size)
 *
 * \see fmt()
 */
template <typename T, size_t dest_size>
inline char *fmt (const T &value, char (&dest)[dest_size])
  noexcept(noexcept(__bits::fmt(value, dest, dest + dest_size)))
{
  return __bits::fmt(value, dest, dest + dest_size);
}


/**
 * Optimized specialization for copying compile-time const \a src[\a src_size]
 * to [\a first, \a last).
 *
 * \see fmt()
 */
template <size_t src_size>
inline char *fmt (const char (&src)[src_size], char *first, char *last)
  noexcept(noexcept(__bits::copy(src, src + src_size - 1, first, last)))
{
  return __bits::copy(src, src + src_size - 1, first, last);
}


/**
 * Optimized specialization for copying compile-time const \a src[\a src_size]
 * to \a dest[\a dest_size].
 *
 * \see fmt()
 */
template <size_t src_size, size_t dest_size>
inline char *fmt (const char (&src)[src_size], char (&dest)[dest_size])
  noexcept(noexcept(__bits::copy(src, src + src_size - 1, dest, dest + dest_size)))
{
  static_assert(src_size - 1 <= dest_size, "not enough room");
  return __bits::copy(src, src + src_size - 1, dest, dest + dest_size);
}


/**
 * View manipulator to copy \a value as hexadecimal human readable
 * representation. Only integral types are valid for \a T.
 *
 * Usage:
 * \code
 * auto end = sal::fmt(sal::hex(42ULL), first, last);
 * \endcode
 *
 * \return Opaque type, do not touch it's internals
 */
template <typename T>
inline auto hex (T value) noexcept
{
  return __bits::hex<T>{value};
}


/**
 * View manipulator to copy \a value as octal human readable
 * representation. Only integral types are valid for \a T.
 *
 * Usage:
 * \code
 * auto end = sal::fmt(sal::oct(42ULL), first, last);
 * \endcode
 *
 * \return Opaque type, do not touch it's internals
 */
template <typename T>
inline auto oct (T value) noexcept
{
  return __bits::oct<T>{value};
}


/**
 * View manipulator to copy \a value as binary human readable
 * representation. Only integral types are valid for \a T.
 *
 * Usage:
 * \code
 * auto end = sal::fmt(sal::bin(42ULL), first, last);
 * \endcode
 *
 * \return Opaque type, do not touch it's internals
 */
template <typename T>
inline auto bin (T value) noexcept
{
  return __bits::bin<T>{value};
}


__sal_end
} // namespace sal
