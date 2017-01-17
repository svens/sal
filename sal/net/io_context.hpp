#pragma once

/**
 * \file sal/net/io_context.hpp
 */


#include <sal/config.hpp>
#include <sal/net/io_buf.hpp>
#include <sal/net/error.hpp>
#include <array>
#include <chrono>
#include <deque>

#include <iostream>


__sal_begin


namespace net {


class io_context_t
{
public:

  io_buf_ptr make_buf ()
  {
    auto *io_buf = free_bufs_.try_pop();
    if (!io_buf)
    {
      extend_pool();
      io_buf = free_bufs_.try_pop();
    }
    io_buf->clear();
    return io_buf_ptr{io_buf, &io_context_t::free_io_buf};
  }


  template <typename Rep, typename Period>
  io_buf_ptr wait (const std::chrono::duration<Rep, Period> &timeout,
    std::error_code &error) noexcept
  {
    io_buf_ptr io_buf{next(), &io_context_t::free_io_buf};
    if (!io_buf && wait(std::chrono::milliseconds(timeout).count(), error))
    {
      io_buf.reset(next());
    }
    return io_buf;
  }


  template <typename Rep, typename Period>
  io_buf_ptr wait (const std::chrono::duration<Rep, Period> &timeout)
  {
    return wait(timeout, throw_on_error("io_context_t::wait"));
  }


private:

  std::deque<std::array<char, 1024 * sizeof(io_buf_t)>> pool_;
  io_buf_t::free_list free_bufs_{};


  void extend_pool ()
  {
    pool_.emplace_back();
    auto &slot = pool_.back();
    char *it = slot.data(), * const e = slot.data() + slot.size();
    size_t n = 0;
    for (/**/;  it != e;  it += sizeof(io_buf_t))
    {
      auto *io_buf = new(it) io_buf_t(this);
      free_bufs_.push(io_buf);
      n++;
    }
  }


  static void free_io_buf (io_buf_t *io_buf) noexcept
  {
    io_buf->owner_->free_bufs_.push(io_buf);
  }


  bool wait (size_t timeout_milliseconds, std::error_code &error) noexcept;
  io_buf_t *next () noexcept;
};


} // namespace net


__sal_end
