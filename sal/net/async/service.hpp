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

  context_t &this_context () const noexcept
  {
    return *reinterpret_cast<context_t *>(impl_->this_context);
  }


  template <typename UserData>
  void user_data (UserData *ptr) noexcept
  {
    impl_->user_data_type = type_v<UserData>;
    impl_->user_data = ptr;
  }


  template <typename UserData>
  UserData *user_data () const noexcept
  {
    if (impl_->user_data_type == type_v<UserData>)
    {
      return static_cast<UserData *>(impl_->user_data);
    }
    return nullptr;
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

  friend class context_t;
};


//
// context
//
class context_t
  : protected __bits::context_t
{
public:

  io_t make_io ()
  {
    return {__bits::context_t::make_io()};
  }


  template <typename UserData>
  io_t make_io (UserData *user_data)
  {
    return {__bits::context_t::make_io(type_v<UserData>, user_data)};
  }


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
