#pragma once

/**
 * \file sal/thread.hpp
 * Complementary threading functionality for C++11
 */


#include <sal/config.hpp>


__sal_begin


/// Thread id type, suitable for \c printf("%u")
using thread_id = uint32_t;


/// Thread id, representing no thread
constexpr thread_id null_thread = 0;


/// Current thread context related functionality
namespace this_thread {


namespace __bits {
thread_id make_id () noexcept;
} // namespace __bits


/**
 * Return current thread id, unique amongst all other threads in process.
 *
 * Main reason for this function is to provide POD-type value for printf-like
 * functions. For all other reasons, \c std::this_thread::get_id() should be
 * used instead.
 */
inline thread_id get_id () noexcept
{
#if __apple_build_version__
  // TODO: fix once Xcode8 is available
  static __thread auto id_ = null_thread;
  if (!id_)
  {
    id_ = __bits::make_id();
  }
#else
  static thread_local const auto id_ = __bits::make_id();
#endif

  return id_;
}


} // namespace this_thread


__sal_end
