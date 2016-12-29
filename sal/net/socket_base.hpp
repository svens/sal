#pragma once

/**
 * \file sal/net/socket_base.hpp
 * Common socket base types and values.
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <system_error>


__sal_begin


namespace net {


/// Common socket types and values.
class socket_base_t
{
public:

  /// Native socket handle type
  using native_handle_t = __bits::native_socket_handle_t;

  /// Invalid native socket handle
  static constexpr native_handle_t no_handle = static_cast<native_handle_t>(-1);


  /// Socket shutdown flags
  enum class shutdown_t
  {
    /// Disables further receive operations
    receive = SHUT_RD,
    /// Disables further send operations
    send = SHUT_WR,
    /// Disables further send and receive operations
    both = SHUT_RDWR,
  };


  /// Socket wait types
  enum class wait_t
  {
    /// Wait until socket is ready to read
    read = 1,
    /// Wait until socket is ready to write
    write = 2,
    /// Wait until socket has a pending error condition
    error = 4,
  };


  /// Bitmask flags for send/receive functions
  enum class message_flags_t
  {
    /// Leave received data in queue
    peek = MSG_PEEK,
    /// Out-of-band data
    out_of_band = MSG_OOB,
    /// Send without using routing tables
    do_not_route = MSG_DONTROUTE,
  };


  /// Limit on length of the queue of pending incoming connections
  static constexpr int max_listen_connections = SOMAXCONN;


protected:

  ~socket_base_t ()
  {}

  static native_handle_t open (int domain,
    int type,
    int protocol,
    std::error_code &error
  ) noexcept;

  static void close (native_handle_t handle,
    std::error_code &error
  ) noexcept;

  static void get_opt (native_handle_t handle,
    int level,
    int name,
    void *data,
    socklen_t *size,
    std::error_code &error
  ) noexcept;

  static void set_opt (native_handle_t handle,
    int level,
    int name,
    const void *data,
    socklen_t size,
    std::error_code &error
  ) noexcept;
};


} // namespace net


__sal_end
