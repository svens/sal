#pragma once

/**
 * \file sal/net/io_context.hpp
 */


#include <sal/config.hpp>
#include <sal/net/__bits/io_service.hpp>
#include <sal/net/io_buf.hpp>
#include <sal/net/error.hpp>
#include <array>
#include <chrono>
#include <deque>


__sal_begin


namespace net {


class io_context_t
  : public __bits::io_context_t
{
public:

  io_context_t (const io_context_t &) = delete;
  io_context_t &operator= (const io_context_t &) = delete;

  io_context_t (io_context_t &&) = default;
  io_context_t &operator= (io_context_t &&) = default;


  io_buf_ptr make_buf ()
  {
    io_buf_ptr io_buf{free_.try_pop(), &io_context_t::free_io_buf};
    if (!io_buf)
    {
      extend_pool();
      io_buf.reset(free_.try_pop());
    }
    io_buf->reset();
    io_buf->context = this;
    return io_buf;
  }


  io_buf_ptr try_get () noexcept
  {
    auto io_buf = __bits::io_context_t::try_get();
    return io_buf_ptr{
      static_cast<io_buf_t *>(io_buf),
      &io_context_t::free_io_buf
    };
  }


  template <typename Rep, typename Period>
  io_buf_ptr get (const std::chrono::duration<Rep, Period> &timeout,
    std::error_code &error) noexcept
  {
    auto io_buf = __bits::io_context_t::get(
      std::chrono::duration_cast<std::chrono::milliseconds>(timeout),
      error
    );
    return io_buf_ptr{
      static_cast<io_buf_t *>(io_buf),
      &io_context_t::free_io_buf
    };
  }


  template <typename Rep, typename Period>
  io_buf_ptr get (const std::chrono::duration<Rep, Period> &timeout)
  {
    return get(timeout, throw_on_error("io_context::get"));
  }


  io_buf_ptr get (std::error_code &error) noexcept
  {
    return get((std::chrono::milliseconds::max)(), error);
  }


  io_buf_ptr get ()
  {
    return get((std::chrono::milliseconds::max)(),
      throw_on_error("io_context::get")
    );
  }


  void reclaim () noexcept
  {
    while (auto *completed = __bits::io_context_t::try_get())
    {
      free_io_buf(static_cast<io_buf_t *>(completed));
    }
  }


private:

  std::deque<std::array<char, 1024 * sizeof(io_buf_t)>> pool_{};
  io_buf_t::free_list free_{};

  io_context_t (__bits::io_service_t &io_service, size_t max_completion_count) noexcept
    : __bits::io_context_t(io_service, max_completion_count)
  {}

  static void free_io_buf (io_buf_t *io_buf) noexcept
  {
    io_buf->owner_->free_.push(io_buf);
  }

  void extend_pool ();

  friend class io_service_t;
};


} // namespace net


__sal_end
