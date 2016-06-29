#pragma once

/**
 * \file sal/spinlock.hpp
 *
 * Spinlock implementation using std::atomic_flag. In most situations,
 * application should just use std::mutex. This lets OS scheduler to decide
 * when waiting for lock has been too long and suspend waiting core. See
 * http://stackoverflow.com/questions/5869825/when-should-one-use-a-spinlock-instead-of-mutex
 * for more information.
 *
 * If for specific situation spinlock still makes sense, this implementation
 * allows to customize loop while waiting by using callable yielding policy as
 * parameter for spinlock_t::lock(SpinYield). There are some pre-implemented
 * policies (busy_spin, yield_spin, adaptive_spin).
 *
 * \note Before deciding whether to use spinlock_t or std::mutex do profiling.
 */

#include <sal/config.hpp>
#include <atomic>
#include <chrono>
#include <thread>


namespace sal {
__sal_begin


/// Busy spinning policy for spinlock_t::lock(SpinYield)
inline void busy_spin (size_t iter_count) noexcept
{
  (void)iter_count;
}


/// Remaining timeslice yielding policy for spinlock_t::lock(SpinYield)
inline void yield_spin (size_t iter_count) noexcept
{
  (void)iter_count;
  std::this_thread::yield();
}


/**
 * Adaptive remaining timeslice yielding policy for spinlock_t::lock(SpinYield).
 *
 * Depending on how many times spinlock_t::lock() has spinned, it yields
 * differently:
 *   - \a iter_count <= \a BusySpinCount
 *     Busy spinning
 *   - \a iter_count > \a BusySpinCount
 *     Yield remaining timeslice
 *   - \a iter_count > 2x \a BusySpinCount
 *     Sleep i.e. let OS scheduler suspend core if it decides so
 */
template <size_t BusySpinCount>
inline void adaptive_spin (size_t iter_count) noexcept
{
  if (iter_count < BusySpinCount)
  {
  }
  else if (iter_count < 2 * BusySpinCount)
  {
    std::this_thread::yield();
  }
  else
  {
    using namespace std::chrono_literals;
    // LCOV_EXCL_BR_START
    std::this_thread::sleep_for(iter_count > 1000 ? 1ms : iter_count * 1us);
    // LCOV_EXCL_BR_STOP
  }
}


/**
 * Non-recursive spinlock using std::atomic_flag
 *
 * This API satisfies C++ Lockable concept i.e. it can be used with
 * std::lock_guard and std::unique_lock.
 */
class spinlock_t
{
public:

  spinlock_t () = default;

  spinlock_t (const spinlock_t &) = delete;
  spinlock_t &operator= (const spinlock_t &) = delete;


  /// Try to lock. Returns true if succeeds immediately, false otherwise.
  bool try_lock () noexcept
  {
    return flag_.test_and_set(std::memory_order_acquire) == false;
  }


  /**
   * Lock spinlock. It calls repeatedly try_lock() until it succeeds. On each
   * failed try_lock(), \a yield(spin_count) is called to let application
   * customize waiting loop.
   */
  template <typename SpinYield>
  void lock (SpinYield yield) noexcept(noexcept(yield(1)))
  {
    for (size_t spin_count = 0;  !try_lock();  ++spin_count)
    {
      yield(spin_count);
    }
  }


  /// Lock spinlock using adaptive_spin<100> as yielding policy
  void lock () noexcept
  {
    lock(adaptive_spin<100>);
  }


  /// Unlock spinlock.
  void unlock () noexcept
  {
    return flag_.clear(std::memory_order_release);
  }


private:

  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};


__sal_end
} // namespace sal
