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

  using native_handle_t = __bits::native_poller_t;


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


  void register_socket (const socket_base_t &socket, uintptr_t socket_data,
    std::error_code &error
  ) noexcept;


  void register_socket (const socket_base_t &socket, uintptr_t socket_data)
  {
    register_socket(socket, socket_data,
      throw_on_error("io_service_t::register_socket")
    );
  }


private:

  __bits::poller_t poller_;
};


} // namespace net


__sal_end
