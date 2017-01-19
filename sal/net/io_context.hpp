#pragma once

/**
 * \file sal/net/io_context.hpp
 */


#include <sal/config.hpp>
#include <sal/net/__bits/async.hpp>
#include <sal/net/io_buf.hpp>
#include <sal/net/error.hpp>
#include <array>
#include <deque>


__sal_begin


namespace net {


class io_context_t
{
public:

  io_context_t (const io_context_t &) = delete;
  io_context_t &operator= (const io_context_t &) = delete;

  io_context_t (io_context_t &&) = default;
  io_context_t &operator= (io_context_t &&) = default;


  io_buf_ptr make_buf () noexcept
  {
    io_buf_ptr io_buf{free_.try_pop(), &io_context_t::free_io_buf};
    if (!io_buf && extend_pool())
    {
      io_buf.reset(free_.try_pop());
    }
    sal_check_ptr(io_buf.get())->clear();
    io_buf->this_context_ = this;
    return io_buf;
  }


  io_buf_ptr try_get () noexcept
  {
    return io_buf_ptr{completed_.try_pop(), &io_context_t::free_io_buf};
  }


  io_buf_ptr get (std::error_code &error) noexcept
  {
    auto io_buf = try_get();
    if (!io_buf && wait_for_more((std::chrono::milliseconds::max)(), error))
    {
      io_buf.reset(completed_.try_pop());
    }
    return io_buf;
  }


  io_buf_ptr get ()
  {
    return get(throw_on_error("io_context_t::get"));
  }


  template <typename Rep, typename Period>
  bool wait (const std::chrono::duration<Rep, Period> &period,
    std::error_code &error) noexcept
  {
    return wait_for_more(period, error);
  }


  template <typename Rep, typename Period>
  bool wait (const std::chrono::duration<Rep, Period> &period)
  {
    return wait(period, throw_on_error("io_context_t::wait"));
  }


  void defer (io_buf_ptr &buf) noexcept
  {
    completed_.push(buf.release());
  }


private:

  __bits::poller_t &poller_;

  std::deque<std::array<char, 1024 * sizeof(io_buf_t)>> pool_{};
  io_buf_t::free_list free_{};

  size_t wait_max_completed_;
  io_buf_t::completed_list completed_{};


  io_context_t (__bits::poller_t &poller, size_t max_wait_completed)
    : poller_(poller)
    , wait_max_completed_(max_wait_completed)
  {}

  bool extend_pool () noexcept;

  static void free_io_buf (io_buf_t *io_buf) noexcept
  {
    io_buf->owner_context_->free_.push(io_buf);
  }

  bool wait_for_more (const std::chrono::milliseconds &period,
    std::error_code &error
  ) noexcept;

  friend class io_service_t;
};


} // namespace net


__sal_end
