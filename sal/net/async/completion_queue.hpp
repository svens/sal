#pragma once

/**
 * \file sal/net/async/completion_queue.hpp
 * Asynchronous I/O operations completion queue.
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/net/async/io.hpp>
#include <sal/net/async/service.hpp>
#include <chrono>


__sal_begin


namespace net::async {


/**
 * Asynchronous networking I/O completion queue.
 * \see sal::net::async::service_t
 */
class completion_queue_t
{
public:

  /**
   * Instantiate new completion queue for \a service.
   */
  completion_queue_t (const service_t &service) noexcept
    : impl_(service.impl_)
  { }


  completion_queue_t (const completion_queue_t &) = delete;
  completion_queue_t (completion_queue_t &&) = delete;
  completion_queue_t &operator= (const completion_queue_t &) = delete;
  completion_queue_t &operator= (completion_queue_t &&) = delete;


  /**
   * Allocate new I/O operation with associated \a context.
   */
  template <typename Context>
  io_ptr make_io (Context *context)
  {
    auto io = reinterpret_cast<io_t *>(impl_.make_io());
    io->context<Context>(context);
    return io_ptr{io};
  }


  /**
   * Allocate new I/O operation.
   */
  io_ptr make_io ()
  {
    return make_io<std::nullptr_t>(nullptr);
  }


  /**
   * Return next completed I/O operation without blocking calling thread. If
   * there is no pending completion immediately available, return nullptr.
   */
  io_ptr try_get () noexcept
  {
    return io_ptr{reinterpret_cast<io_t *>(impl_.try_get())};
  }


  /**
   * Suspend calling thread up to \a timeout until there are more I/O
   * operations completed. After successful wait, next try_get() is guaranteed
   * to return completed I/O operation. On polling failure, set \a error.
   *
   * \returns true if there were completed I/O operations, false otherwise.
   */
  template <typename Rep, typename Period>
  bool wait_for (const std::chrono::duration<Rep, Period> &timeout,
    std::error_code &error) noexcept
  {
    return impl_.wait(timeout, error);
  }


  /**
   * Suspend calling thread up to \a timeout until there are more I/O
   * operations completed. After successful wait, next try_get() is guaranteed
   * to return completed I/O operation.
   *
   * \returns true if there were completed I/O operations, false otherwise.
   * \throws std::system_error on polling failure.
   */
  template <typename Rep, typename Period>
  bool wait_for (const std::chrono::duration<Rep, Period> &timeout)
  {
    return wait_for(timeout, throw_on_error("service::wait_for"));
  }


  /**
   * Suspend calling thread until there are more I/O operations completed.
   * After successful wait, next try_get() is guaranteed to return completed
   * I/O operation. On polling failure, set \a error.
   *
   * \returns true if there were completed I/O operations, false otherwise.
   */
  bool wait (std::error_code &error) noexcept
  {
    return wait_for((std::chrono::milliseconds::max)(), error);
  }


  /**
   * Suspend calling thread until there are more I/O operations completed.
   * After successful wait, next try_get() is guaranteed to return completed
   * I/O operation.
   *
   * \returns true if there were completed I/O operations, false otherwise.
   * \throws std::system_error on polling failure.
   */
  bool wait ()
  {
    return wait(throw_on_error("service::wait"));
  }


  /**
   * Poll for new completed I/O operations. On polling failure, set \a error.
   * \returns true if there were completed I/O operations, false otherwise.
   */
  bool poll (std::error_code &error) noexcept
  {
    return wait_for(std::chrono::milliseconds::zero(), error);
  }


  /**
   * Poll for new completed I/O operations.
   * \throws std::system_error on polling failure.
   */
  bool poll ()
  {
    return poll(throw_on_error("service::poll"));
  }


private:

  __bits::completion_queue_t impl_;
};


} // namespace net::async


__sal_end
