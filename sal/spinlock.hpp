#pragma once

/**
 * \file sal/spinlock.hpp
 * Spinlock
 */

#include <sal/config.hpp>
#include <sal/__bits/spinlock.hpp>


namespace sal {
__sal_begin


/**
 * Wrapper for platform's provided spinlock or atomic flag if none.
 *
 * Applications should use spinlock only for very short-lived locks because
 * when blocked, calling thread will keep spinning, preventing OS scheduler to
 * suspend thread while waiting.
 *
 * Spinlock is not recursive.
 *
 * This API satisfies C++ Lockable concept i.e. it can be used with
 * std::lock_guard and std::unique_lock.
 */
class spinlock
{
public:

  spinlock () = default;

  spinlock (const spinlock &) = delete;
  spinlock &operator= (const spinlock &) = delete;


  /// Try to lock. Returns true if succeeds immediately, false otherwise.
  bool try_lock () noexcept
  {
    return __bits::spinlock_try_lock(&lock_);
  }


  /**
   * Lock spinlock. If this method does not succeed immediately, it keeps
   * spinning and trying to acquire lock during each spin. On first \a count
   * iterations it simply spins and later yields it's timeslice on each failed
   * lock attempt.
   */
  void lock (size_t count = 100) noexcept
  {
    __bits::spinlock_lock(&lock_, count);
  }


  /// Unlock spinlock.
  void unlock () noexcept
  {
    __bits::spinlock_unlock(&lock_);
  }


private:

  __bits::native_spinlock lock_{};
};


__sal_end
} // namespace sal
