#include <sal/net/io_service.hpp>


__sal_begin


namespace net {


io_service_t::io_service_t (size_t max_concurrency)
{
#if __sal_os_windows

  poller_ = ::CreateIoCompletionPort(__bits::invalid_poller,
    NULL,
    0,
    static_cast<DWORD>(max_concurrency)
  );

  if (poller_ == __bits::invalid_poller)
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


void io_service_t::associate (__bits::native_socket_t socket,
  std::error_code &error) noexcept
{
#if __sal_os_windows

  auto result = ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket),
    poller_,
    0,
    0
  );
  if (!result)
  {
    error.assign(::GetLastError(), std::system_category());
  }

#else

  (void)socket;
  (void)error;

#endif
}


} // namespace net


__sal_end
