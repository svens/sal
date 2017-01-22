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


void poller_t::associate (native_socket_t socket,
  uintptr_t socket_data,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  auto result = ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket),
    handle,
    socket_data,
    0
  );
  if (!result)
  {
    error.assign(::GetLastError(), std::system_category());
  }

#else

  (void)socket;
  (void)socket_data;
  (void)error;

#endif
}


size_t poller_t::wait (const std::chrono::milliseconds &timeout,
  poller_record_t entries[], size_t max_entries,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  ULONG completed_count;
  auto succeeded = GetQueuedCompletionStatusEx(handle,
    entries, static_cast<ULONG>(max_entries),
    &completed_count,
    static_cast<DWORD>(timeout.count()),
    false
  );

  if (succeeded)
  {
    return completed_count;
  }

  auto e = ::GetLastError();
  if (e != WAIT_TIMEOUT)
  {
    error.assign(e, std::system_category());
  }
  return 0;

#else

  (void)timeout;
  (void)entries;
  (void)max_entries;
  (void)error;
  return 0;

#endif
}


}} // namespace net::__bits


__sal_end
