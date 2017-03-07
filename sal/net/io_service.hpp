#pragma once

/**
 * \file sal/net/io_service.hpp
 */


#include <sal/config.hpp>
#include <sal/net/__bits/io_service.hpp>
#include <sal/net/basic_socket.hpp>
#include <sal/net/basic_socket_acceptor.hpp>
#include <sal/net/error.hpp>
#include <sal/net/io_context.hpp>


__sal_begin


namespace net {


class io_service_t
{
public:

  io_service_t ()
    : impl_(throw_on_error("io_service"))
  {}


  io_context_t make_context (size_t max_events_per_wait = 16)
  {
    if (max_events_per_wait < 1)
    {
      max_events_per_wait = 1;
    }
    else if (max_events_per_wait > impl_.max_events_per_wait)
    {
      max_events_per_wait = impl_.max_events_per_wait;
    }
    return io_context_t(impl_, max_events_per_wait);
  }


  template <typename Protocol>
  void associate (basic_socket_t<Protocol> &socket, std::error_code &error)
    noexcept
  {
    impl_.associate(socket.impl_, error);
  }


  template <typename Protocol>
  void associate (basic_socket_t<Protocol> &socket)
  {
    associate(socket, throw_on_error("io_service::associate"));
  }


  template <typename Protocol>
  void associate (basic_socket_acceptor_t<Protocol> &socket,
    std::error_code &error) noexcept
  {
    impl_.associate(socket.impl_, error);
  }


  template <typename Protocol>
  void associate (basic_socket_acceptor_t<Protocol> &socket)
  {
    associate(socket, throw_on_error("io_service::associate"));
  }


private:

  __bits::io_service_t impl_;
};


} // namespace net


__sal_end
