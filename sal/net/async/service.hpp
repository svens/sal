#pragma once

/**
 * \file sal/net/async/service.hpp
 * Asynchronous networking service
 */


#include <sal/config.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/net/error.hpp>
#include <sal/type_id.hpp>


__sal_begin


namespace net::async {


class service_t;
class context_t;
class io_t;
class datagram_socket_t;


//
// io
//
class io_t
{
public:

  template <typename UserData>
  void user_data (UserData *ptr) noexcept
  {
    impl_->user_data = ptr;
  }


  void *user_data () const noexcept
  {
    return impl_->user_data;
  }


  template <typename AsyncResult>
  const AsyncResult *result_for (std::error_code &error) noexcept
  {
    error.clear();
    return nullptr;
  }


  template <typename AsyncResult>
  const AsyncResult *result_for ()
  {
    return result_for<AsyncResult>(throw_on_error("io::result_for"));
  }


private:

  __bits::io_ptr impl_;

  io_t (__bits::io_t *impl) noexcept
    : impl_(impl)
  { }

  friend class service_t;
};


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

  __bits::service_ptr impl_ = std::make_shared<__bits::service_t>(
    throw_on_error("service_t")
  );
};


//
// datagram socket
//
class datagram_socket_t
{
public:

  struct receive_from_t
  {
    datagram_socket_t *socket;
    size_t transferred;
  };

  void start_receive_from (io_t &&io) noexcept;


private:
};


} // namespace net::async


__sal_end
