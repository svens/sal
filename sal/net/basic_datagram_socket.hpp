#pragma once

/**
 * \file sal/net/basic_socket.hpp
 * Datagram socket
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/basic_socket.hpp>


__sal_begin


namespace net {


/**
 * Datagram socket
 */
template <typename Protocol>
class basic_datagram_socket_t
  : public basic_socket_t<Protocol>
{
public:

  /// Socket's low-level handle
  using native_handle_t = typename basic_socket_t<Protocol>::native_handle_t;

  /// Socket's protocol.
  using protocol_t = typename basic_socket_t<Protocol>::protocol_t;

  /// Socket's endpoint
  using endpoint_t = typename basic_socket_t<Protocol>::endpoint_t;


  basic_datagram_socket_t () = default;


  basic_datagram_socket_t (basic_datagram_socket_t &&that) noexcept
    : basic_socket_t<Protocol>(std::move(that))
  {}


  basic_datagram_socket_t (const protocol_t &protocol)
    : basic_socket_t<Protocol>(protocol)
  {}


  basic_datagram_socket_t (const endpoint_t &endpoint)
    : basic_socket_t<Protocol>(endpoint)
  {}


  basic_datagram_socket_t (const protocol_t &protocol,
      const native_handle_t &handle)
    : basic_socket_t<Protocol>(protocol, handle)
  {}


  basic_datagram_socket_t &operator= (basic_datagram_socket_t &&that) noexcept
  {
    basic_socket_t<Protocol>::operator=(std::move(that));
    return *this;
  }


  basic_datagram_socket_t (const basic_datagram_socket_t &) = delete;
  basic_datagram_socket_t &operator= (const basic_datagram_socket_t &) = delete;


#if 0

  size_t receive (buffer, error);
  size_t receive (buffer);

  size_t receive (buffer, flags, error);
  size_t receive (buffer, flags);

  size_t receive_from (buffer, endpoint, error);
  size_t receive_from (buffer, endpoint);

  size_t receive_from (buffer, endpoint, flags, error);
  size_t receive_from (buffer, endpoint, flags);

#endif
};


} // namespace net


__sal_end
