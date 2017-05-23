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
  using handle_t = __bits::socket_t::handle_t;

  /// Invalid native socket handle
  static constexpr handle_t invalid = __bits::socket_t::invalid;


  /// \{
  /// Socket shutdown types
  using shutdown_t = __bits::shutdown_t;
  /// Disables further receive operations
  static constexpr shutdown_t shutdown_receive = shutdown_t::receive;
  /// Disables further send operations
  static constexpr shutdown_t shutdown_send = shutdown_t::send;
  /// Disables further send and receive operations
  static constexpr shutdown_t shutdown_both = shutdown_t::both;
  /// \}


  /// \{
  /// Socket state change waiting types
  using wait_t = __bits::wait_t;
  /// Wait for socket become readable
  static constexpr wait_t wait_read = wait_t::read;
  /// Wait for socket become writable
  static constexpr wait_t wait_write = wait_t::write;
  /// \}


  /// \{
  /// Bitmask flags for send/receive functions
  using message_flags_t = __bits::message_flags_t;
  /// Leave received data in queue
  static constexpr message_flags_t peek = MSG_PEEK;
  /// Out-of-band data
  static constexpr message_flags_t out_of_band = MSG_OOB;
  /// Send without using routing tables
  static constexpr message_flags_t do_not_route = MSG_DONTROUTE;
  /// \}


  /// Limit on length of the queue of pending incoming connections
  static constexpr int max_listen_connections = SOMAXCONN;


protected:

  ~socket_base_t ()
  {}
};


} // namespace net


__sal_end
