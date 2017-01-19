#pragma once

#include <sal/config.hpp>
#include <sal/net/__bits/socket.hpp>
#include <chrono>


__sal_begin


namespace net { namespace __bits {


#if __sal_os_windows

  using native_poller_t = HANDLE;
  constexpr native_poller_t invalid_poller = INVALID_HANDLE_VALUE;

  using poller_record_t = OVERLAPPED_ENTRY;

  using io_buf_aux_t = OVERLAPPED;
  inline void reset (io_buf_aux_t &aux) noexcept
  {
    std::memset(&aux, '\0', sizeof(aux));
  }

#else

  using native_poller_t = int;
  constexpr native_poller_t invalid_poller = -1;

  using poller_record_t = int;

  struct io_buf_aux_t {};
  inline void reset (io_buf_aux_t &) noexcept {}

#endif


struct poller_t
{
  native_poller_t handle = invalid_poller;

  poller_t (size_t max_concurrency);
  ~poller_t () noexcept;

  void associate (native_socket_t socket,
    uintptr_t socket_data,
    std::error_code &error
  ) noexcept;

  size_t wait (const std::chrono::milliseconds &timeout,
    poller_record_t entries[], size_t max_entries,
    std::error_code &error
  ) noexcept;
};


}} // namespace net::__bits


__sal_end

