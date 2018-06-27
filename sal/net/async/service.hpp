#pragma once

/**
 * \file sal/net/async/service.hpp
 * Asynchronous networking service
 */


#include <sal/config.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/net/async/io.hpp>


__sal_begin


namespace net::async {


class service_t;
class context_t;


//
// context
//
class context_t
  : protected __bits::context_t
{
public:


private:

  context_t (__bits::service_ptr service, size_t max_events_per_poll)
    : __bits::context_t(service, max_events_per_poll)
  { }

  friend class service_t;
};


//
// service
//
class service_t
{
public:

  service_t (size_t completion_queue_size)
    : impl_(std::make_shared<__bits::service_t>(completion_queue_size))
  { }


  context_t make_context (size_t max_events_per_poll = 16)
  {
    return {impl_, max_events_per_poll};
  }


  io_t make_io ()
  {
    return {impl_->make_io()};
  }


  template <typename UserData>
  io_t make_io (UserData *ptr)
  {
    return {impl_->make_io(ptr)};
  }


  size_t io_pool_size () const noexcept
  {
    return impl_->io_pool_size;
  }


private:

  __bits::service_ptr impl_;
};


} // namespace net::async


__sal_end
