#pragma once

/**
 * \file sal/net/socket_options.hpp
 * Socket option getters and setters.
 *
 * Usage:
 * \code
 * some_socket_t socket;
 *
 * // getting SO_KEEPALIVE flag
 * bool keepalive;
 * socket.get_option(sal::net::keep_alive(&keepalive));
 * if (keep_alive) ...
 *
 * // set SO_KEEPALIVE off
 * socket.set_option(sal:net::keep_alive(false));
 * \endcode
 */


#include <sal/config.hpp>
#include <sal/net/__bits/socket.hpp>
#include <chrono>


__sal_begin


namespace net {


namespace __bits {


template <int Level, int Name, typename Type>
struct socket_option_setter_t
{
  static constexpr int level = Level;
  static constexpr int name = Name;

  using native_t = int;
  Type data;

  socket_option_setter_t (Type value) noexcept
    : data(value)
  {}

  void store (native_t &value) const noexcept
  {
    value = data;
  }
};


inline void assign (bool &dest, int source) noexcept
{
  dest = !!source;
}


inline void assign (int &dest, int source) noexcept
{
  dest = source;
}


template <int Level, int Name, typename Type>
struct socket_option_getter_t
{
  static constexpr int level = Level;
  static constexpr int name = Name;

  using native_t = int;
  Type *data;

  socket_option_getter_t (Type *value) noexcept
    : data(value)
  {}

  void load (const native_t &value, size_t) const noexcept
  {
    assign(*data, value);
  }
};


} // namespace __bits


/**
 * Set whether a socket permits sending of broadcast messages (if supported by
 * protocol).
 *
 * \note Valid only for datagram sockets.
 */
inline auto broadcast (bool value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_BROADCAST, bool>
{
  return value;
}


/**
 * Query whether a socket permits sending of broadcast messages, (if supported
 * by protocol).
 *
 * \note Valid only for datagram sockets.
 */
inline auto broadcast (bool *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_BROADCAST, bool>
{
  return value;
}


/**
 * Set whether debugging information is recorded by the underlying protocol.
 *
 * \note On Linux, it is allowed only for processes with CAP_NET_ADMIN
 * capability or an effective user with ID of 0.
 */
inline auto debug (bool value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_DEBUG, bool>
{
  return value;
}


/**
 * Query whether debugging information is recorded by the underlying protocol.
 *
 * \note On Linux, it is allowed only for processes with CAP_NET_ADMIN
 * capability or an effective user with ID of 0.
 */
inline auto debug (bool *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_DEBUG, bool>
{
  return value;
}


/**
 * Set whether outgoing messages bypass standard routing facilities.
 *
 * \note Not supported on Windows platforms.
 */
inline auto do_not_route (bool value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_DONTROUTE, bool>
{
  return value;
}


/**
 * Query whether outgoing messages bypass standard routing facilities.
 *
 * \note Not supported on Windows platforms.
 */
inline auto do_not_route (bool *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_DONTROUTE, bool>
{
  return value;
}


/**
 * Set whether a socket permits sending of keep-alive messages (if supported
 * by the protocol)
 *
 * \note Valid only for connection-oriented sockets
 * \note Can't change on Windows platforms
 */
inline auto keep_alive (bool value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_KEEPALIVE, bool>
{
  return value;
}


/**
 * Query whether a socket permits sending of keep-alive messages (if supported
 * by the protocol)
 *
 * \note Valid only for connection-oriented sockets
 */
inline auto keep_alive (bool *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_KEEPALIVE, bool>
{
  return value;
}


/**
 * Set whether the validation of endpoint used for binding socket should allow
 * the reuse of local endpoints (if supported by protocol).
 */
inline auto reuse_address (bool value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_REUSEADDR, bool>
{
  return value;
}


/**
 * Query whether the validation of endpoint used for binding socket should
 * allow the reuse of local endpoints (if supported by protocol).
 */
inline auto reuse_address (bool *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_REUSEADDR, bool>
{
  return value;
}


#if SO_REUSEPORT


/**
 * Set whether to allow duplicate bindings by multiple processes if they all
 * set this options before bind(2).
 */
inline auto reuse_port (bool value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_REUSEPORT, bool>
{
  return value;
}


/**
 * Query whether duplicate bindings are allowed for socket.
 */
inline auto reuse_port (bool *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_REUSEPORT, bool>
{
  return value;
}


#endif // SO_REUSEPORT


/**
 * Set the size of receive buffer associated with socket.
 */
inline auto receive_buffer_size (int value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_RCVBUF, int>
{
  return value;
}


/**
 * Query the size of receive buffer associated with socket.
 */
inline auto receive_buffer_size (int *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_RCVBUF, int>
{
  return value;
}


/**
 * Specify the minimum number of bytes to process for socket input operations.
 *
 * \note Not changeable on Linux and Windows platforms
 */
inline auto receive_low_watermark (int value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_RCVLOWAT, int>
{
  return value;
}


/**
 * Query the minimum number of bytes to process for socket input operations.
 */
inline auto receive_low_watermark (int *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_RCVLOWAT, int>
{
  return value;
}


/**
 * Set the size of send buffer associated with socket.
 */
inline auto send_buffer_size (int value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_SNDBUF, int>
{
  return value;
}


/**
 * Query the size of send buffer associated with socket.
 */
inline auto send_buffer_size (int *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_SNDBUF, int>
{
  return value;
}


/**
 * Specify the minimum number of bytes to process for socket output
 * operations.
 *
 * \note Not changeable on Linux and Windows platforms
 */
inline auto send_low_watermark (int value) noexcept
  -> __bits::socket_option_setter_t<SOL_SOCKET, SO_SNDLOWAT, int>
{
  return value;
}


/**
 * Query the minimum number of bytes to process for socket output operations.
 */
inline auto send_low_watermark (int *value) noexcept
  -> __bits::socket_option_getter_t<SOL_SOCKET, SO_SNDLOWAT, int>
{
  return value;
}


namespace __bits {


struct socket_option_linger_setter_t
{
  static constexpr int level = SOL_SOCKET;
  static constexpr int name = SO_LINGER;

  using native_t = ::linger;
  bool on;
  std::chrono::seconds timeout;

  socket_option_linger_setter_t (bool on, std::chrono::seconds timeout) noexcept
    : on(on)
    , timeout(timeout)
  {}

  void store (native_t &value) const noexcept
  {
    value.l_onoff = on;
    value.l_linger = static_cast<decltype(value.l_linger)>(timeout.count());
  }
};


struct socket_option_linger_getter_t
{
  static constexpr int level = SOL_SOCKET;
  static constexpr int name = SO_LINGER;

  using native_t = ::linger;
  bool *on;
  std::chrono::seconds *timeout;

  socket_option_linger_getter_t (bool *on, std::chrono::seconds *timeout)
    noexcept
    : on(on)
    , timeout(timeout)
  {}

  void load (const native_t &value, size_t) const noexcept
  {
    *on = !!value.l_onoff;
    *timeout = std::chrono::seconds(value.l_linger);
  }
};


} // namespace __bits


/**
 * Set the behaviour when socket is closed and unsent data is present.
 */
inline auto linger (bool on, std::chrono::seconds timeout) noexcept
  -> __bits::socket_option_linger_setter_t
{
  return __bits::socket_option_linger_setter_t{ on, timeout };
}


/**
 * Query the behaviour when socket is closed and unsent data is present.
 */
inline auto linger (bool *on, std::chrono::seconds *timeout) noexcept
  -> __bits::socket_option_linger_getter_t
{
  return __bits::socket_option_linger_getter_t{ on, timeout };
}


} // namespace net


__sal_end
