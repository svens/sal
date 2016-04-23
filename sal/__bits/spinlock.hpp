#pragma once

#include <sal/config.hpp>


// * Darwin: OSSpinLock; fluctuates less than atomic spinlock
// * Linux: spinlock; fluctuates less than pthread_spinlock_t
// * Windows: spinlock; critical section is recursive unlike others


#if __sal_os_darwin
  #include <libkern/OSAtomic.h>
#else
  #include <atomic>
  #include <chrono>
  #include <thread>
#endif


namespace sal {
__sal_begin


namespace __bits {


#if __sal_os_darwin


using native_spinlock = OSSpinLock;


inline bool spinlock_try_lock (native_spinlock *lock) noexcept
{
  return OSSpinLockTry(lock);
}


inline void spinlock_lock (native_spinlock *lock, size_t count) noexcept
{
  (void)count;
  OSSpinLockLock(lock);
}


inline void spinlock_unlock (native_spinlock *lock) noexcept
{
  OSSpinLockUnlock(lock);
}


#else // __sal_os_darwin


struct native_spinlock
{
  std::atomic_flag flag = ATOMIC_FLAG_INIT;
};


inline bool spinlock_try_lock (native_spinlock *lock) noexcept
{
  return lock->flag.test_and_set(std::memory_order_acquire) == false;
}


inline void spinlock_lock (native_spinlock *lock, size_t count) noexcept
{
  const auto spin_count = count, sleep_count = 2 * count;

  for (auto i = 0U;  !spinlock_try_lock(lock);  ++i)
  {
    if (i > sleep_count)
    {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(i > 1000 ? 1ms : i * 1us);
    }
    else if (i > spin_count)
    {
      std::this_thread::yield();
    }
  }
}


inline void spinlock_unlock (native_spinlock *lock) noexcept
{
  return lock->flag.clear(std::memory_order_release);
}


#endif


} // namespace __bits


__sal_end
} // namespace sal
