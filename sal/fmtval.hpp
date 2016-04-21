#pragma once

/**
 * \file sal/fmtval.hpp
 * Functions to format value to textual representation.
 */


#include <sal/config.hpp>
#include <sal/__bits/fmtval.hpp>


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


__sal_end
} // namespace sal
