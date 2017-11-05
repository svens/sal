#pragma once

/**
 * \file sal/assert.hpp
 * SAL's implementation of assert.
 */

#include <sal/config.hpp>
#include <sal/error.hpp>


__sal_begin


/**
 * \def sal_assert(condition)
 *
 * Check \a condition to be true. On false, throw std::logic_error. Like
 * \c assert, it is invoked only if \c NDEBUG is not defined. If \a condition
 * has desirable side-effects, use sal_verify() instead that evaluations
 * always \c condition.
 */

#if !defined(NDEBUG)
  #define sal_assert(condition) \
    sal::__bits::check(sal_likely(condition), \
      __sal_at ": Assertion '" #condition "' failed" \
    )
#else
  #define sal_assert(condition) ((void)0)
#endif


/**
 * \def sal_verify(condition)
 *
 * Check \a condition to be true. On false, throw std::logic_error. It is
 * similar to sal_assert() except \a condition is always evaluated. This is
 * useful when \a condition has desired side-effects.
 */
#define sal_verify(condition) \
  sal::__bits::check(condition, \
    __sal_at ": Assertion '" #condition "' failed" \
  )


/**
 * \def sal_check_ptr(ptr)
 * Check \a ptr being not null and return \a ptr if true. On null, throw
 * std::logic_error
 */
#define sal_check_ptr(ptr) \
  sal::__bits::check_ptr(ptr, __sal_at ": '" #ptr "' is null")


namespace __bits {


inline void check (bool cond, const char msg[])
{
#if !defined(NDEBUG)
  if (!cond)
  {
    throw_logic_error(msg);
  }
#else
  (void)cond;
  (void)msg;
#endif
}


template <typename T>
inline T *check_ptr (T *ptr, const char msg[])
{
#if !defined(NDEBUG)
  if (!ptr)
  {
    throw_logic_error(msg);
  }
#else
  (void)msg;
#endif
  return ptr;
}


inline void *check_ptr (std::nullptr_t, const char msg[])
{
  return check_ptr<void>(nullptr, msg);
}


} // namespace __bits


__sal_end
