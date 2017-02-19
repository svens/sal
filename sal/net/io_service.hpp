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


#if __sal_os_windows
__sal_begin


namespace net {


class io_service_t
{
public:

  io_service_t ()
    : impl_(throw_on_error("io_service"))
  {}


  io_context_t make_context (size_t completion_count = 16)
  {
    if (completion_count < 1)
    {
      completion_count = 1;
    }
    else if (completion_count > impl_.max_completion_count)
    {
      completion_count = impl_.max_completion_count;
    }
    return io_context_t(impl_, completion_count);
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
#endif // __sal_os_windows
