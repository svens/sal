#pragma once

/**
 * \file sal/error.hpp
 * SAL errors
 */

#include <sal/config.hpp>
#include <sal/c_str.hpp>
#include <stdexcept>
#include <system_error>


namespace sal {
__sal_begin


/// Throw std::logic_error with message composed from list of \a args.
template <typename... Args>
inline void throw_logic_error [[noreturn]] (Args &&...args)
{
  c_str<128*sizeof...(Args)> what;
  print(what, std::forward<Args>(args)...);
  throw std::logic_error(what.get()); // LCOV_EXCL_BR_LINE
}


/// Throw std::runtime_error with message composed from list of \a args.
template <typename... Args>
inline void throw_runtime_error [[noreturn]] (Args &&...args)
{
  c_str<128*sizeof...(Args)> what;
  print(what, std::forward<Args>(args)...);
  throw std::runtime_error(what.get()); // LCOV_EXCL_BR_LINE
}


/// Throw std::system_error with message composed from list of \a args.
template <typename... Args>
inline void throw_system_error [[noreturn]] (const std::error_code &error,
  Args &&...args)
{
  c_str<128*sizeof...(Args)> what;
  print(what, std::forward<Args>(args)...);
  throw std::system_error(error, what.get()); // LCOV_EXCL_BR_LINE
}


__sal_end
} // namespace sal
