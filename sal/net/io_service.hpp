#pragma once

/**
 * \file sal/net/io_service.hpp
 *
 * Asynchronous I/O completion service.
 */


#include <sal/config.hpp>
#include <sal/net/basic_socket.hpp>
#include <sal/net/basic_socket_acceptor.hpp>
#include <sal/net/error.hpp>


__sal_begin


namespace net {


class io_service_t
{
public:

  template <typename Protocol>
  void associate (basic_socket_t<Protocol> &socket, std::error_code &error)
    noexcept
  {
    associate(socket.socket_, error);
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
    associate(socket.socket_, error);
  }


  template <typename Protocol>
  void associate (basic_socket_acceptor_t<Protocol> &socket)
  {
    associate(socket, throw_on_error("io_service::associate"));
  }


private:

  void associate (__bits::socket_t &socket, std::error_code &error) noexcept;
};


} // namespace net


__sal_end
