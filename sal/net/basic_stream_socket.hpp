#pragma once

/**
 * \file sal/net/basic_stream_socket.hpp
 * Stream socket
 */


#include <sal/config.hpp>
#include <sal/buf_ptr.hpp>
#include <sal/net/basic_socket.hpp>


__sal_begin


namespace net {


/**
 * Stream socket
 *
 * For more information about asynchronous API usage, see io_service_t.
 */
template <typename Protocol>
class basic_stream_socket_t
  : public basic_socket_t<Protocol>
{
  using base_t = basic_socket_t<Protocol>;

public:

  /// Socket's low-level handle
  using handle_t = typename base_t::handle_t;

  /// Socket's protocol.
  using protocol_t = typename base_t::protocol_t;

  /// Socket's endpoint
  using endpoint_t = typename base_t::endpoint_t;


  basic_stream_socket_t () = default;


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
   * Initialise base class using \a handle
   */
  basic_stream_socket_t (handle_t handle)
    : base_t(handle)
  {}


  //
  // Synchronous API
  //


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received. On failure, set \a error and return 0.
   */
  template <typename Ptr>
  size_t receive (const Ptr &buf,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    return base_t::socket_.receive(buf.data(), buf.size(),
      static_cast<int>(flags),
      error
    );
  }


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received. On failure, throw std::system_error
   */
  template <typename Ptr>
  size_t receive (const Ptr &buf, socket_base_t::message_flags_t flags)
  {
    return receive(buf, flags, throw_on_error("basic_stream_socket::receive"));
  }


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received. On failure, set \a error and return 0.
   */
  template <typename Ptr>
  size_t receive (const Ptr &buf, std::error_code &error) noexcept
  {
    return receive(buf, socket_base_t::message_flags_t{}, error);
  }


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received. On failure, throw std::system_error
   */
  template <typename Ptr>
  size_t receive (const Ptr &buf)
  {
    return receive(buf, throw_on_error("basic_stream_socket::receive"));
  }


  /**
   * Write data of \a buf into this socket for delivering to connected
   * endpoint. On success, returns number of bytes sent. On failure, set
   * \a error and return 0.
   */
  template <typename Ptr>
  size_t send (const Ptr &buf,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    return base_t::socket_.send(buf.data(), buf.size(),
      static_cast<int>(flags),
      error
    );
  }


  /**
   * Write data of \a buf into this socket for delivering to connected
   * endpoint. On success, returns number of bytes sent. On failure, throw
   * std::system_error
   */
  template <typename Ptr>
  size_t send (const Ptr &buf, socket_base_t::message_flags_t flags)
  {
    return send(buf, flags, throw_on_error("basic_stream_socket::send"));
  }


  /**
   * Write data of \a buf into this socket for delivering to connected
   * endpoint. On success, returns number of bytes sent. On failure, set
   * \a error and return 0.
   */
  template <typename Ptr>
  size_t send (const Ptr &buf, std::error_code &error) noexcept
  {
    return send(buf, socket_base_t::message_flags_t{}, error);
  }


  /**
   * Write data of \a buf into this socket for delivering to connected
   * endpoint. On success, returns number of bytes sent. On failure, throw
   * std::system_error
   */
  template <typename Ptr>
  size_t send (const Ptr &buf)
  {
    return send(buf, throw_on_error("basic_stream_socket::send"));
  }
};


} // namespace net


__sal_end
