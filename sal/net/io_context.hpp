#pragma once

/**
 * \file sal/net/io_context.hpp
 *
 * Per-thread io_service_t representative.
 */


#include <sal/config.hpp>
#include <sal/net/__bits/async_socket.hpp>
#include <sal/net/io_buf.hpp>
#include <sal/net/error.hpp>
#include <array>
#include <chrono>
#include <deque>


__sal_begin


namespace net {


/**
 * Per I/O thread representative of io_service_t. It also maintains per-thread
 * resources (io_buf_t pool, etc). Each instance of io_context_t maintains own
 * operations and completions queues.
 *
 * To wait for completions, call get() repeatedly that returns next completion
 * from queue. If queue is empty, it uses io_service_t to get batch of next
 * completions that'll be returned one by one to application.
 *
 * This class is not meant to be instantiated directly but through
 * io_service_t::make_context(). Instances must be kept alive until all of
 * buffers allocated using make_buf() are finished and returned to owner
 * thread's io_context_t pool.
 */
class io_context_t
  : public __bits::io_context_t
{
public:

  io_context_t (const io_context_t &) = delete;
  io_context_t &operator= (const io_context_t &) = delete;

  /// Move ctor
  io_context_t (io_context_t &&) = default;

  /// Move assign
  io_context_t &operator= (io_context_t &&) = default;


  /**
   * Allocate handle (io_buf_t) for asynchronous operation. It first tries to
   * return handle from internal pool of free handles. On exhaustion, next
   * batch of handles are allocated and one of them is returned. On allocation
   * failure, internal ```std::dequeue::emplace_back()``` throws an exception.
   */
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


  /**
   * Return next asynchronous operation completion or \c nullptr if none. This
   * method does not block or wait for next batch of completions.
   */
  io_buf_ptr try_get () noexcept
  {
    auto io_buf = __bits::io_context_t::try_get();
    return io_buf_ptr{
      static_cast<io_buf_t *>(io_buf),
      &io_context_t::free_io_buf
    };
  }


  /**
   * Return next asynchronous operation completion. If none is immediately
   * ready, this method waits for more completions until \a timeout has
   * passed. If still no completions, \c nullptr is returned.
   *
   * On failure, \a error is set.
   */
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


  /**
   * Return next asynchronous operation completion. If none is immediately
   * ready, this method waits for more completions until \a timeout has
   * passed. If still no completions, \c nullptr is returned.
   *
   * On failure, std::system_error is thrown.
   */
  template <typename Rep, typename Period>
  io_buf_ptr get (const std::chrono::duration<Rep, Period> &timeout)
  {
    return get(timeout, throw_on_error("io_context::get"));
  }


  /**
   * Return next asynchronous operation completion. If none is immediately
   * ready, this method waits indefinitely until next completion is ready to
   * be returned.
   *
   * On failure, \a error is set.
   */
  io_buf_ptr get (std::error_code &error) noexcept
  {
    return get((std::chrono::milliseconds::max)(), error);
  }


  /**
   * Return next asynchronous operation completion. If none is immediately
   * ready, this method waits indefinitely until next completion is ready to
   * be returned.
   *
   * On failure, std::system_error is thrown.
   */
  io_buf_ptr get ()
  {
    return get((std::chrono::milliseconds::max)(),
      throw_on_error("io_context::get")
    );
  }


  /**
   * Iterate and release all already completed asynchronous operations.
   * \returns number of completions released.
   */
  size_t reclaim () noexcept
  {
    auto count = 0;
    while (auto *io_buf = __bits::io_context_t::try_get())
    {
      free_io_buf(static_cast<io_buf_t *>(io_buf));
      ++count;
    }
    return count;
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
