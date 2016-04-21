#pragma once

/**
 * \file sal/assert.hpp
 * SAL's implementation of assert: sal_expect for pre-condition and
 * sal_ensure for post-condition checking.
 */

#include <sal/config.hpp>
#include <stdexcept>


namespace sal {
__sal_begin


/**
 * \def sal_expect(cond)
 * Check pre-condition \a cond to be true. On false, throw std::logic_error
 */
#define sal_expect(cond) \
  sal::__bits::check(cond, __sal_at ": Assertion '" #cond "' failed")


/**
 * \def sal_ensure(cond)
 * Check post-condition \a cond to be true. On false, throw std::logic_error
 */
#define sal_ensure(cond) \
  sal::__bits::check(cond, __sal_at ": Assertion '" #cond "' failed")


/**
 * \def sal_check_ptr(ptr)
 * Check \a ptr being not null and return \a ptr if true. On null, throw
 * std::logic_error
 */
#define sal_check_ptr(ptr) \
  sal::__bits::check_ptr(ptr, __sal_at ": '" #ptr "' is null")


namespace __bits {


inline void check (bool cond, const char *msg)
{
#if !defined(NDEBUG)
  if (!cond)
  {
    throw std::logic_error(msg);
  }
#else
  (void)cond;
  (void)msg;
#endif
}


template <typename T>
inline T *check_ptr (T *ptr, const char *msg)
{
#if !defined(NDEBUG)
  if (!ptr)
  {
    throw std::logic_error(msg);
  }
#else
  (void)msg;
#endif
  return ptr;
}


inline void *check_ptr (std::nullptr_t, const char *msg)
{
  return check_ptr<void>(nullptr, msg);
}


} // namespace __bits


__sal_end
} // namespace sal
