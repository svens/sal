#pragma once

/**
 * \file sal/net/async/worker.hpp
 */


#include <sal/config.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/net/async/io.hpp>


__sal_begin


namespace net::async {


class worker_t
{
public:

  io_t try_get () noexcept
  {
    return {impl_.try_get()};
  }


  template <typename Rep, typename Period>
  io_t poll (const std::chrono::duration<Rep, Period> &timeout,
    std::error_code &error) noexcept
  {
    return {impl_.poll(timeout, error)};
  }


  template <typename Rep, typename Period>
  io_t poll (const std::chrono::duration<Rep, Period> &timeout)
  {
    return poll(timeout, throw_on_error("worker::poll"));
  }


  io_t poll (std::error_code &error) noexcept
  {
    return poll((std::chrono::milliseconds::max)(), error);
  }


  io_t poll ()
  {
    return poll((std::chrono::milliseconds::max)());
  }


  io_t try_poll (std::error_code &error) noexcept
  {
    return poll(std::chrono::milliseconds(0), error);
  }


  io_t try_poll ()
  {
    return poll(std::chrono::milliseconds(0));
  }


  size_t reclaim () noexcept
  {
    auto count = 0U;
    for (/**/;  try_get();  ++count) /**/;
    return count;
  }


private:

  __bits::worker_t impl_;

  worker_t (__bits::service_ptr service, size_t max_results_per_poll) noexcept
    : impl_(service, max_results_per_poll)
  {}

  friend class service_t;
};


} // namespace net::async


__sal_end
