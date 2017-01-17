#pragma once

/**
 * \file sal/net/io_service.hpp
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/error.hpp>
#include <sal/net/io_context.hpp>


__sal_begin


namespace net {


class io_service_t
{
public:

  io_service_t (size_t max_concurrency = 0);


  io_context_t make_context ();


  void register_socket (const socket_base_t &socket, uintptr_t socket_data,
    std::error_code &error
  ) noexcept;


  void register_socket (const socket_base_t &socket, uintptr_t socket_data)
  {
    register_socket(socket, socket_data,
      throw_on_error("io_service_t::register_socket")
    );
  }
};


} // namespace net


__sal_end
