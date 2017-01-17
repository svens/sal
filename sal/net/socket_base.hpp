#pragma once

/**
 * \file sal/net/socket_base.hpp
 * Common socket base types and values.
 */


#include <sal/config.hpp>
#include <sal/net/__bits/socket.hpp>


__sal_begin


namespace net {


/// Common socket types and values.
class socket_base_t
{
public:

  /// Native socket handle type
  using native_handle_t = __bits::native_socket_t;

  /// Invalid native socket handle
  static constexpr native_handle_t invalid_socket = __bits::invalid_socket;


  /// Socket shutdown types
  using shutdown_t = int;

  /// Disables further receive operations
  static constexpr shutdown_t shutdown_receive = SHUT_RD;
  /// Disables further send operations
  static constexpr shutdown_t shutdown_send = SHUT_WR;
  /// Disables further send and receive operations
  static constexpr shutdown_t shutdown_both = SHUT_RDWR;


  /// Socket waiting types
  using wait_t = __bits::wait_t;

  /// Wait for socket become readable
  static constexpr wait_t wait_read = __bits::wait_t::read;
  /// Wait for socket become writable
  static constexpr wait_t wait_write = __bits::wait_t::write;


  /// Bitmask flags for send/receive functions
  using message_flags_t = int;

  /// Leave received data in queue
  static constexpr message_flags_t peek = MSG_PEEK;
  /// Out-of-band data
  static constexpr message_flags_t out_of_band = MSG_OOB;
  /// Send without using routing tables
  static constexpr message_flags_t do_not_route = MSG_DONTROUTE;


  /// Limit on length of the queue of pending incoming connections
  static constexpr int max_listen_connections = SOMAXCONN;


protected:

  ~socket_base_t ()
  {}
};


} // namespace net


__sal_end
