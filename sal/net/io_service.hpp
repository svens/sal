#pragma once

/**
 * \file sal/net/io_service.hpp
 */


#include <sal/config.hpp>
#include <sal/net/__bits/socket.hpp>
#include <sal/net/error.hpp>
#include <sal/net/io_context.hpp>


__sal_begin


namespace net {


class io_service_t
{
public:

  io_service_t (size_t max_concurrency = 0);


  io_context_t make_context (size_t max_wait_completed = 16)
  {
    if (max_wait_completed < 1)
    {
      max_wait_completed = 1;
    }
    else if (max_wait_completed > 64)
    {
      max_wait_completed = 64;
    }
    return io_context_t(poller_, max_wait_completed);
  }


  template <typename Socket>
  void associate (const Socket &socket, uintptr_t socket_data,
    std::error_code &error) noexcept
  {
    associate(socket.native_handle(), socket_data, error);
  }


  template <typename Socket>
  void associate (const Socket &socket, uintptr_t socket_data = 0)
  {
    associate(socket, socket_data, throw_on_error("io_service_t::associate"));
  }


private:

  __bits::native_poller_t poller_ = __bits::invalid_poller;

  void associate (__bits::native_socket_t socket,
    uintptr_t socket_data,
    std::error_code &error
  ) noexcept;
};


} // namespace net


__sal_end
