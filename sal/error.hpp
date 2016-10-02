#pragma once

/**
 * \file sal/error.hpp
 * SAL errors
 */

#include <sal/config.hpp>
#include <sal/str.hpp>
#include <stdexcept>
#include <system_error>
#include <type_traits>


namespace sal {
__sal_begin


template <typename Error, typename... Args>
inline void throw_error [[noreturn]] (Args &&...args)
{
  static_assert(std::is_base_of<std::exception, Error>::value,
    "exceptions should be inherited from std::exception"
  );
  str_t<1 + 128*sizeof...(Args)> what;
  print(what, std::forward<Args>(args)...);
  throw Error(what.get());
}


/// Throw std::logic_error with message composed from list of \a args.
template <typename... Args>
inline void throw_logic_error [[noreturn]] (Args &&...args)
{
  str_t<1 + 128*sizeof...(Args)> what;
  print(what, std::forward<Args>(args)...);
  throw std::logic_error(what.get());
}


/// Throw std::runtime_error with message composed from list of \a args.
template <typename... Args>
inline void throw_runtime_error [[noreturn]] (Args &&...args)
{
  str_t<1 + 128*sizeof...(Args)> what;
  print(what, std::forward<Args>(args)...);
  throw std::runtime_error(what.get());
}


/// Throw std::system_error with message composed from list of \a args.
template <typename... Args>
inline void throw_system_error [[noreturn]] (const std::error_code &error,
  Args &&...args)
{
  str_t<1 + 128*sizeof...(Args)> what;
  print(what, std::forward<Args>(args)...);
  throw std::system_error(error, what.get());
}


__sal_end
} // namespace sal
