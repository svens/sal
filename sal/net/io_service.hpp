#pragma once

/**
 * \file sal/net/io_service.hpp
 */


#include <sal/config.hpp>
#include <sal/net/__bits/async.hpp>
#include <sal/net/error.hpp>
#include <sal/net/io_context.hpp>


__sal_begin


namespace net {


class io_service_t
{
public:

  io_service_t (size_t max_concurrency = 0)
    : poller_(max_concurrency)
  {}


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
    poller_.associate(socket.native_handle(), socket_data, error);
  }


  template <typename Socket>
  void associate (const Socket &socket, uintptr_t socket_data)
  {
    associate(socket, socket_data, throw_on_error("io_service_t::associate"));
  }


private:

  __bits::poller_t poller_;
};


} // namespace net


__sal_end
