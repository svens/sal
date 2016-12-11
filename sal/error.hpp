#pragma once

/**
 * \file sal/error.hpp
 * SAL errors
 */

#include <sal/config.hpp>
#include <sal/char_array.hpp>
#include <stdexcept>
#include <system_error>
#include <type_traits>


__sal_begin


/// Throw \a Error with message composed from list of \a args.
template <typename Error, typename... Args>
inline void throw_error [[noreturn]] (Args &&...args)
{
  static_assert(std::is_base_of<std::exception, Error>::value,
    "exceptions should be inherited from std::exception"
  );
  char_array_t<1 + 128*sizeof...(Args)> what;
  what.print(std::forward<Args>(args)...);
  throw Error(what.c_str());
}


/// Throw std::logic_error with message composed from list of \a args.
template <typename... Args>
inline void throw_logic_error [[noreturn]] (Args &&...args)
{
  char_array_t<1 + 128*sizeof...(Args)> what;
  what.print(std::forward<Args>(args)...);
  throw std::logic_error(what.c_str());
}


/// Throw std::runtime_error with message composed from list of \a args.
template <typename... Args>
inline void throw_runtime_error [[noreturn]] (Args &&...args)
{
  char_array_t<1 + 128*sizeof...(Args)> what;
  what.print(std::forward<Args>(args)...);
  throw std::runtime_error(what.c_str());
}


/// Throw std::system_error with message composed from list of \a args.
template <typename... Args>
inline void throw_system_error [[noreturn]] (const std::error_code &error,
  Args &&...args)
{
  char_array_t<1 + 128*sizeof...(Args)> what;
  what.print(std::forward<Args>(args)...);
  throw std::system_error(error, what.c_str());
}


__sal_end
