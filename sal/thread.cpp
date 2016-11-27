#include <sal/thread.hpp>

#if __sal_os_darwin
  #include <pthread.h>
#elif __sal_os_linux
  #include <unistd.h>
  #include <sys/syscall.h>
#elif __sal_os_windows
  #include <Windows.h>
#else
  #error Unsupported platform
#endif


__sal_begin


namespace this_thread { namespace __bits {


thread_id make_id () noexcept
{
#if __sal_os_darwin
  return pthread_mach_thread_np(pthread_self());
#elif __sal_os_linux
  return syscall(SYS_gettid);
#elif __sal_os_windows
  return GetCurrentThreadId();
#endif
}


}} // namespace this_thread::__bits


__sal_end
