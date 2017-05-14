#pragma once

/**
 * \file sal/error.hpp
 * SAL errors
 */

#include <sal/config.hpp>
#include <sal/builtins.hpp>
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


/**
 * Helper class to catch std::error_code and throw std::system_error if there
 * was an error.
 */
class throw_on_error
{
public:

  throw_on_error () = delete;
  throw_on_error (const throw_on_error &) = delete;
  throw_on_error &operator= (const throw_on_error &) = delete;

  /// Construct error thrower with message \a msg
  throw_on_error (const char *msg) noexcept
    : msg_(msg)
  {}

  ~throw_on_error () noexcept(false)
  {
    if (code_)
    {
      throw_system_error(code_, msg_);
    }
  }

  operator std::error_code& () noexcept
  {
    return code_;
  }

private:

  std::error_code code_{};
  const char * const msg_;
};


/**
 * \def sal_throw_if(condition)
 *
 * If \a condition is true, throw logic_error with \a condition as message
 * prepended with source location (file:line)
 */
#define sal_throw_if(condition) \
  sal::__bits::throw_if(condition, \
    __sal_at ": Failed because '" #condition "'" \
  )


namespace __bits {

template <size_t MsgSize>
inline void throw_if (bool condition, const char (&msg)[MsgSize])
{
  if (sal_unlikely(condition))
  {
    throw_logic_error(msg);
  }
}

} // namespace __bits


__sal_end
