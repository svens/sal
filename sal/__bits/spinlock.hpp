#pragma once

#include <sal/config.hpp>

#if __sal_os_darwin
  #include <libkern/OSAtomic.h>
  #include <atomic>
  #include <chrono>
  #include <thread>
#else
  #error Unsupported platform
#endif


namespace sal {
__sal_begin


namespace __bits {


#if defined(__sal_os_darwin)


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


#endif


} // namespace __bits


__sal_end
} // namespace sal
