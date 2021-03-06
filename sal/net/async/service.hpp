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
 * basic_socket_acceptor_t::associate()).
 *
 * After socket launches any asynchronous I/O operation, it's completions are
 * reported using sal::net::async::completion_queue_t.
 *
 * service_t also maintains internally pool of free io_t objects. Any started
 * I/O operation must be allocated from initiator socket's associated
 * service_t pool. Starting I/O using another service_t pool's io_t is
 * undefined behavior.
 *
 * Typical completed I/O operations handling loop may look like:
 * \code
 * sal::net::async::completion_queue_t io_queue(io_svc);
 * while (!stop_requested)
 * {
 *   if (auto io = io_queue.try_get())
 *   {
 *     // handle completed io
 *   }
 *   else
 *   {
 *     io_queue.wait_for(1s);
 *   }
 * }
 * \endcode
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
   * Allocate new I/O operation with associated \a context.
   */
  template <typename Context>
  io_ptr make_io (Context *context)
  {
    auto io = reinterpret_cast<io_t *>(impl_->make_io());
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


private:

  __bits::service_ptr impl_ = std::make_shared<__bits::service_t>();

  friend class completion_queue_t;
  template <typename Protocol> friend class net::basic_socket_t;
  template <typename Protocol> friend class net::basic_socket_acceptor_t;
};


} // namespace net::async


__sal_end
