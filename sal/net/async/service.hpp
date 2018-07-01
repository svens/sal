#pragma once

/**
 * \file sal/net/async/service.hpp
 * Asynchronous networking service
 */


#include <sal/config.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/net/async/io.hpp>
#include <sal/net/async/worker.hpp>


__sal_begin


namespace net::async {


class service_t
{
public:

  service_t (size_t completion_queue_size)
    : impl_(std::make_shared<__bits::service_t>(completion_queue_size))
  { }


  worker_t make_worker (size_t max_results_per_poll) noexcept
  {
    return {impl_, max_results_per_poll};
  }


  io_t make_io ()
  {
    return {impl_->make_io()};
  }


  template <typename Context>
  io_t make_io (Context *context)
  {
    return {impl_->make_io(context, type_v<Context>)};
  }


  size_t io_pool_size () const noexcept
  {
    return impl_->io_pool_size;
  }


private:

  __bits::service_ptr impl_;

  template <typename Protocol>
  friend class basic_socket_t;
};


} // namespace net::async


__sal_end
