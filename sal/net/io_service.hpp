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


  io_context_t make_context (size_t completion_count = 16)
  {
    if (completion_count < 1)
    {
      completion_count = 1;
    }
    else if (completion_count > io_context_t::max_completion_count)
    {
      completion_count = io_context_t::max_completion_count;
    }
    return io_context_t(poller_, completion_count);
  }


  template <typename Socket>
  void associate (const Socket &socket, std::error_code &error) noexcept
  {
    associate(socket.native_handle(), error);
  }


  template <typename Socket>
  void associate (const Socket &socket)
  {
    associate(socket, throw_on_error("io_service_t::associate"));
  }


private:

  __bits::native_poller_t poller_ = __bits::invalid_poller;

  void associate (__bits::native_socket_t socket, std::error_code &error)
    noexcept;
};


} // namespace net


__sal_end
