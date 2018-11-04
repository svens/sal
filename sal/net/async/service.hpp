#pragma once

/**
 * \file sal/net/async/service.hpp
 * Asynchronous networking service
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/net/async/io.hpp>
#include <chrono>


__sal_begin


namespace net::async {


/**
 * Asynchronous networking I/O completion service.
 *
 * This class holds OS-dependent completion handler (IOCP/epoll/kqueue). Each
 * socket that requests to start asynchronous operation, must first be
 * associated with service_t (see basic_socket_t::associate() and
 * basic_socket_acceptor_t::associate()). After socket launches any
 * asynchronous operation, it's completion is reported using this class
 * (wait_for(), wait(), try_get()).
 *
 * service_t also maintains internally pool of free io_t objects. Any started
 * I/O operation must be allocated from initiator socket's associated
 * service_t pool. Starting I/O using another service_t pool's io_t is
 * undefined behavior.
 */
class service_t
{
public:

  /**
   * Return number of internally allocated I/O operation instances (both in
   * use and available).
   */
  size_t io_pool_size () const noexcept
  {
    return impl_->io_pool_size;
  }


  /**
   * Allocate new I/O operation.
   */
  io_t make_io ()
  {
    return {impl_->make_io(nullptr, type_v<std::nullptr_t>)};
  }


  /**
   * Allocate new I/O operation with associated \a context.
   */
  template <typename Context>
  io_t make_io (Context *context)
  {
    return {impl_->make_io(context, type_v<Context>)};
  }


  /**
   * Return next completed I/O operation without blocking calling thread. If
   * there is no pending completion immediately available, do not poll
   * underlying OS handle but return empty I/O instance.
   */
  io_t try_get () noexcept
  {
    return {impl_->dequeue()};
  }


  /**
   * Return next completed I/O operation. If there is no pending completion
   * immediately available, suspend calling thread up to \a timeout. If after
   * \a timeout there is still no pending completion, returns empty I/O
   * instance.
   *
   * On polling failure, set \a error
   */
  template <typename Rep, typename Period>
  io_t wait_for (const std::chrono::duration<Rep, Period> &timeout,
    std::error_code &error) noexcept
  {
    do
    {
      if (auto io = try_get())
      {
        return io;
      }
    } while (impl_->wait_for_more(timeout, error));

    return {nullptr};
  }


  /**
   * Return next completed I/O operation. If there is no pending completion
   * immediately available, suspend calling thread up to \a timeout. If after
   * \a timeout there is still no pending completion, returns empty I/O
   * instance.
   *
   * \throws std::system_error on polling failure.
   */
  template <typename Rep, typename Period>
  io_t wait_for (const std::chrono::duration<Rep, Period> &timeout)
  {
    return wait_for(timeout, throw_on_error("service::wait_for"));
  }


  /**
   * Return next completed I/O operation. If there is no pending completion
   * immediately available, suspend calling thread until any completion.
   *
   * On polling failure, set \a error
   */
  io_t wait (std::error_code &error) noexcept
  {
    return wait_for((std::chrono::milliseconds::max)(), error);
  }


  /**
   * Return next completed I/O operation. If there is no pending completion
   * immediately available, suspend calling thread until any completion.
   *
   * \throws std::system_error on polling failure.
   */
  io_t wait ()
  {
    return wait_for((std::chrono::milliseconds::max)());
  }


  /**
   * Return next completed I/O operation. If there is no pending completion
   * available, poll underlying OS handle with immediate timeout.
   *
   * On polling failure, set \a error
   */
  io_t poll (std::error_code &error) noexcept
  {
    return wait_for(std::chrono::milliseconds::zero(), error);
  }


  /**
   * Return next completed I/O operation. If there is no pending completion
   * available, poll underlying OS handle with immediate timeout.
   *
   * \throws std::system_error on polling failure.
   */
  io_t poll ()
  {
    return wait_for(std::chrono::milliseconds::zero());
  }


private:

  __bits::service_ptr impl_ = std::make_shared<__bits::service_t>();

  template <typename Protocol> friend class net::basic_socket_t;
  template <typename Protocol> friend class net::basic_socket_acceptor_t;
};


} // namespace net::async


__sal_end
