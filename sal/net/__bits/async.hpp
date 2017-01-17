#pragma once

#include <sal/config.hpp>
#include <sal/net/__bits/socket.hpp>


__sal_begin


namespace net { namespace __bits {


#if __sal_os_windows

  using native_poller_t = HANDLE;
  constexpr native_poller_t invalid_poller = INVALID_HANDLE_VALUE;

  using io_buf_aux_t = OVERLAPPED;
  inline void reset (io_buf_aux_t &aux) noexcept
  {
    std::memset(&aux, '\0', sizeof(aux));
  }

#else

  using native_poller_t = int;
  constexpr native_poller_t invalid_poller = -1;

  struct io_buf_aux_t {};
  inline void reset (io_buf_aux_t &) noexcept {}

#endif


struct poller_t
{
  native_poller_t handle = invalid_poller;

  poller_t (size_t max_concurrency);
  ~poller_t () noexcept;
};


}} // namespace net::__bits


__sal_end

