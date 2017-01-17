#include <sal/net/__bits/async.hpp>
#include <sal/net/error.hpp>


__sal_begin


namespace net { namespace __bits {


poller_t::poller_t (size_t max_concurrency)
{
#if __sal_os_windows

  handle = ::CreateIoCompletionPort(invalid_poller,
    NULL,
    0,
    static_cast<DWORD>(max_concurrency)
  );

  if (handle == invalid_poller)
  {
    throw_system_error(
      std::error_code(::GetLastError(), std::system_category()),
      "io_service_t"
    );
  }

#else

  (void)max_concurrency;

#endif
}


poller_t::~poller_t () noexcept
{
#if __sal_os_windows

  ::CloseHandle(handle);

#endif
}


}} // namespace net::__bits


__sal_end
