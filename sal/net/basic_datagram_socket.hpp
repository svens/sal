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
  using base_t = basic_socket_t<Protocol>;

public:

  /// Socket's low-level handle
  using native_handle_t = typename base_t::native_handle_t;

  /// Socket's protocol.
  using protocol_t = typename base_t::protocol_t;

  /// Socket's endpoint
  using endpoint_t = typename base_t::endpoint_t;


  basic_datagram_socket_t () = default;


  basic_datagram_socket_t (basic_datagram_socket_t &&that) noexcept
    : base_t(std::move(that))
  {}


  basic_datagram_socket_t (const protocol_t &protocol)
    : base_t(protocol)
  {}


  basic_datagram_socket_t (const endpoint_t &endpoint)
    : base_t(endpoint)
  {}


  basic_datagram_socket_t (const protocol_t &protocol,
      const native_handle_t &handle)
    : base_t(protocol, handle)
  {}


  basic_datagram_socket_t &operator= (basic_datagram_socket_t &&that) noexcept
  {
    base_t::operator=(std::move(that));
    return *this;
  }


  basic_datagram_socket_t (const basic_datagram_socket_t &) = delete;
  basic_datagram_socket_t &operator= (const basic_datagram_socket_t &) = delete;


  //
  // receive
  //

  size_t receive_from (char *data, size_t size,
    endpoint_t &endpoint,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    size_t endpoint_size = endpoint.capacity();
    size = __bits::recv_from(base_t::native_handle(),
      data, size,
      endpoint.data(), &endpoint_size,
      static_cast<int>(flags),
      error
    );
    if (!error)
    {
      endpoint.resize(endpoint_size);
    }
    return size;
  }


  size_t receive_from (char *data, size_t max_size,
    endpoint_t &endpoint,
    socket_base_t::message_flags_t flags)
  {
    return receive_from(data, max_size,
      endpoint, flags,
      throw_on_error("basic_datagram_socket::receive_from")
    );
  }


  size_t receive_from (char *data, size_t max_size,
    endpoint_t &endpoint,
    std::error_code &error) noexcept
  {
    return receive_from(data, max_size,
      endpoint, socket_base_t::message_flags_t{},
      error
    );
  }


  size_t receive_from (char *data, size_t max_size, endpoint_t &endpoint)
  {
    return receive_from(data, max_size, endpoint,
      throw_on_error("basic_datagram_socket::receive_from")
    );
  }



  //
  // send
  //

  size_t send_to (const char *data, size_t size,
    const endpoint_t &endpoint,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    return __bits::send_to(base_t::native_handle(),
      data, size,
      endpoint.data(), endpoint.size(),
      static_cast<int>(flags),
      error
    );
  }


  size_t send_to (const char *data, size_t size,
    const endpoint_t &endpoint,
    socket_base_t::message_flags_t flags)
  {
    return send_to(data, size, endpoint, flags,
      throw_on_error("basic_datagram_socket::send_to")
    );
  }


  size_t send_to (const char *data, size_t size,
    const endpoint_t &endpoint,
    std::error_code &error) noexcept
  {
    return send_to(data, size, endpoint,
      socket_base_t::message_flags_t{},
      error
    );
  }


  size_t send_to (const char *data, size_t size,
    const endpoint_t &endpoint)
  {
    return send_to(data, size, endpoint,
      throw_on_error("basic_datagram_socket::send_to")
    );
  }


#if 0

  //
  // receive
  //


  size_t receive (buffer, error);
  size_t receive (buffer);

  size_t receive (buffer, flags, error);
  size_t receive (buffer, flags);

  //
  // send
  //


  size_t send (buffer, error);
  size_t send (buffer);

  size_t send (buffer, flags, error);
  size_t send (buffer, flags);

#endif
};


} // namespace net


__sal_end
