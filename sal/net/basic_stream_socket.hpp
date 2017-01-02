#pragma once

/**
 * \file sal/net/basic_stream_socket.hpp
 * Stream socket
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/basic_socket.hpp>


__sal_begin


namespace net {


/**
 * Stream socket
 */
template <typename Protocol>
class basic_stream_socket_t
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


  basic_stream_socket_t () = default;


  /**
   * Initialise base class from \a that.
   */
  basic_stream_socket_t (basic_stream_socket_t &&that) noexcept
    : base_t(std::move(that))
  {}


  /**
   * Initialise base class using \a protocol
   */
  basic_stream_socket_t (const protocol_t &protocol)
    : base_t(protocol)
  {}


  /**
   * Initialise base class using \a endpoint
   */
  basic_stream_socket_t (const endpoint_t &endpoint)
    : base_t(endpoint)
  {}


  /**
   * Initialise base class using \a endpoint and \a handle
   */
  basic_stream_socket_t (const protocol_t &protocol,
      const native_handle_t &handle)
    : base_t(protocol, handle)
  {}


  /**
   * If this is_open(), close() it and then move all internal resource from
   * \a that to \a this.
   */
  basic_stream_socket_t &operator= (basic_stream_socket_t &&that) noexcept
  {
    base_t::operator=(std::move(that));
    return *this;
  }


  basic_stream_socket_t (const basic_stream_socket_t &) = delete;
  basic_stream_socket_t &operator= (const basic_stream_socket_t &) = delete;


  /**
   * Receive data (up to \a size bytes) from this socket. On success, returns
   * number of bytes received. On failure, set \a error and return 0.
   */
  size_t receive (char *data, size_t size,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    size = __bits::recv(base_t::native_handle(),
      data, size, static_cast<int>(flags), error
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
      throw_on_error("basic_stream_socket::receive")
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
      throw_on_error("basic_stream_socket::receive")
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
    return __bits::send(base_t::native_handle(),
      data, size, static_cast<int>(flags), error
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
      throw_on_error("basic_stream_socket::send")
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
    return send(data, size, throw_on_error("basic_stream_socket::send"));
  }
};


} // namespace net


__sal_end
