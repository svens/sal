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


  /**
   * Initialise base class from \a that.
   */
  basic_datagram_socket_t (basic_datagram_socket_t &&that) noexcept
    : base_t(std::move(that))
  {}


  /**
   * Initialise base class using \a protocol
   */
  basic_datagram_socket_t (const protocol_t &protocol)
    : base_t(protocol)
  {}


  /**
   * Initialise base class using \a endpoint
   */
  basic_datagram_socket_t (const endpoint_t &endpoint)
    : base_t(endpoint)
  {}


  /**
   * Initialise base class using \a endpoint and \a handle
   */
  basic_datagram_socket_t (const protocol_t &protocol,
      const native_handle_t &handle)
    : base_t(protocol, handle)
  {}


  /**
   * If this is_open(), close() it and then move all internal resource from
   * \a that to \a this.
   */
  basic_datagram_socket_t &operator= (basic_datagram_socket_t &&that) noexcept
  {
    base_t::operator=(std::move(that));
    return *this;
  }


  basic_datagram_socket_t (const basic_datagram_socket_t &) = delete;
  basic_datagram_socket_t &operator= (const basic_datagram_socket_t &) = delete;


  /**
   * Receive data (up to \a size bytes) from this socket. On success, returns
   * number of bytes received and stores sender address into \a endpoint.
   * On failure, set \a error and return 0.
   */
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


  /**
   * Receive data (up to \a size bytes) from this socket. On success, returns
   * number of bytes received and stores sender address into \a endpoint.
   * On failure, throw std::system_error.
   */
  size_t receive_from (char *data, size_t max_size,
    endpoint_t &endpoint,
    socket_base_t::message_flags_t flags)
  {
    return receive_from(data, max_size,
      endpoint, flags,
      throw_on_error("basic_datagram_socket::receive_from")
    );
  }


  /**
   * Receive data (up to \a size bytes) from this socket. On success, returns
   * number of bytes received and stores sender address into \a endpoint.
   * On failure, set \a error and return 0.
   */
  size_t receive_from (char *data, size_t max_size,
    endpoint_t &endpoint,
    std::error_code &error) noexcept
  {
    return receive_from(data, max_size,
      endpoint, socket_base_t::message_flags_t{},
      error
    );
  }


  /**
   * Receive data (up to \a size bytes) from this socket. On success, returns
   * number of bytes received and stores sender address into \a endpoint.
   * On failure, throw std::system_error.
   */
  size_t receive_from (char *data, size_t max_size, endpoint_t &endpoint)
  {
    return receive_from(data, max_size, endpoint,
      throw_on_error("basic_datagram_socket::receive_from")
    );
  }


  /**
   * Receive data (up to \a size bytes) from this socket. On success, returns
   * number of bytes received. On failure, set \a error and return 0.
   */
  size_t receive (char *data, size_t size,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    size_t endpoint_size = 0;
    size = __bits::recv_from(base_t::native_handle(),
      data, size,
      nullptr, &endpoint_size,
      static_cast<int>(flags),
      error
    );
    return size;
  }


  /**
   * Receive data (up to \a size bytes) from this socket. On success, returns
   * number of bytes received. On failure, throw std::system_error
   */
  size_t receive (char *data, size_t max_size,
    socket_base_t::message_flags_t flags)
  {
    return receive(data, max_size, flags,
      throw_on_error("basic_datagram_socket::receive")
    );
  }


  /**
   * Receive data (up to \a size bytes) from this socket. On success, returns
   * number of bytes received. On failure, set \a error and return 0.
   */
  size_t receive (char *data, size_t max_size,
    std::error_code &error) noexcept
  {
    return receive(data, max_size,
      socket_base_t::message_flags_t{},
      error
    );
  }


  /**
   * Receive data (up to \a size bytes) from this socket. On success, returns
   * number of bytes received. On failure, throw std::system_error
   */
  size_t receive (char *data, size_t max_size)
  {
    return receive(data, max_size,
      throw_on_error("basic_datagram_socket::receive")
    );
  }


  /**
   * Write \a size bytes of \a data into this socket for delivering to
   * \a endpoint. On success, returns number of bytes sent. On failure, set
   * \a error and return 0.
   */
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


  /**
   * Write \a size bytes of \a data into this socket for delivering to
   * \a endpoint. On success, returns number of bytes sent. On failure, throw
   * std::system_error
   */
  size_t send_to (const char *data, size_t size,
    const endpoint_t &endpoint,
    socket_base_t::message_flags_t flags)
  {
    return send_to(data, size, endpoint, flags,
      throw_on_error("basic_datagram_socket::send_to")
    );
  }


  /**
   * Write \a size bytes of \a data into this socket for delivering to
   * \a endpoint. On success, returns number of bytes sent. On failure, set
   * \a error and return 0.
   */
  size_t send_to (const char *data, size_t size,
    const endpoint_t &endpoint,
    std::error_code &error) noexcept
  {
    return send_to(data, size, endpoint,
      socket_base_t::message_flags_t{},
      error
    );
  }


  /**
   * Write \a size bytes of \a data into this socket for delivering to
   * \a endpoint. On success, returns number of bytes sent. On failure, throw
   * std::system_error
   */
  size_t send_to (const char *data, size_t size,
    const endpoint_t &endpoint)
  {
    return send_to(data, size, endpoint,
      throw_on_error("basic_datagram_socket::send_to")
    );
  }


  /**
   * Write \a size bytes of \a data into this socket for delivering to
   * connected endpoint. On success, returns number of bytes sent. On failure,
   * set \a error and return 0.
   */
  size_t send (const char *data, size_t size,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    return __bits::send_to(base_t::native_handle(),
      data, size,
      nullptr, 0,
      static_cast<int>(flags),
      error
    );
  }


  /**
   * Write \a size bytes of \a data into this socket for delivering to
   * connected endpoint. On success, returns number of bytes sent. On failure,
   * throw std::system_error
   */
  size_t send (const char *data, size_t size,
    socket_base_t::message_flags_t flags)
  {
    return send(data, size, flags,
      throw_on_error("basic_datagram_socket::send")
    );
  }


  /**
   * Write \a size bytes of \a data into this socket for delivering to
   * connected endpoint. On success, returns number of bytes sent. On failure,
   * set \a error and return 0.
   */
  size_t send (const char *data, size_t size, std::error_code &error) noexcept
  {
    return send(data, size, socket_base_t::message_flags_t{}, error);
  }


  /**
   * Write \a size bytes of \a data into this socket for delivering to
   * connected endpoint. On success, returns number of bytes sent. On failure,
   * throw std::system_error
   */
  size_t send (const char *data, size_t size)
  {
    return send(data, size, throw_on_error("basic_datagram_socket::send"));
  }
};


} // namespace net


__sal_end
