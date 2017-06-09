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
 * For more information about asynchronous API usage, see async_service_t.
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


  //
  // Asynchronous API
  //


  /**
   * async_connect() result.
   */
  struct async_connect_t
    : public __bits::async_connect_t
  {};


  /**
   * Start asynchronous async_connect()
   * \see connect()
   */
  void async_connect (io_ptr &&io, const endpoint_t &endpoint) noexcept
  {
    __bits::async_connect_t::start(io.release(), base_t::socket_,
      endpoint.data(), endpoint.size()
    );
  }


  /**
   * Extract and return pointer to async_connect() result from \a io
   * handle or nullptr if \a io does not represent async_connect()
   * operation.
   * If asynchronous connect has failed, set \a error.
   */
  static const async_connect_t *async_connect_result (
    const io_ptr &io, std::error_code &error) noexcept
  {
    return static_cast<async_connect_t *>(
      __bits::async_connect_t::result(io.get(), error)
    );
  }


  /**
   * Extract and return pointer to async_connect() result from \a io
   * handle or nullptr if \a io does not represent async_connect()
   * operation.
   * If asynchronous connect has failed, throw std::system_error
   */
  static const async_connect_t *async_connect_result (const io_ptr &io)
  {
    return async_connect_result(io,
      throw_on_error("basic_stream_socket::async_connect")
    );
  }


  /**
   * async_receive() result.
   */
  struct async_receive_t
    : public __bits::async_receive_t
  {
    /**
     * Number of bytes received.
     */
    size_t transferred () const noexcept
    {
      return __bits::async_receive_t::transferred;
    }
  };


  /**
   * Start asynchronous async_receive()
   * \see receive()
   */
  void async_receive (io_ptr &&io, socket_base_t::message_flags_t flags)
    noexcept
  {
    __bits::async_receive_t::start(io.release(), base_t::socket_, flags);
  }


  /**
   * Start asynchronous async_receive()
   * \see receive()
   */
  void async_receive (io_ptr &&io) noexcept
  {
    async_receive(std::move(io), socket_base_t::message_flags_t{});
  }


  /**
   * Extract and return pointer to async_receive() result from \a io
   * handle or nullptr if \a io does not represent async_receive()
   * operation.
   * If asynchronous receive has failed, set \a error.
   */
  static const async_receive_t *async_receive_result (
    const io_ptr &io, std::error_code &error) noexcept
  {
    return static_cast<async_receive_t *>(
      __bits::async_receive_t::result(io.get(), error)
    );
  }


  /**
   * Extract and return pointer to async_receive() result from \a io
   * handle or nullptr if \a io does not represent async_receive()
   * operation.
   * If asynchronous receive has failed, throw std::system_error
   */
  static const async_receive_t *async_receive_result (const io_ptr &io)
  {
    return async_receive_result(io,
      throw_on_error("basic_stream_socket::async_receive")
    );
  }


  /**
   * async_send() result.
   */
  struct async_send_t
    : public __bits::async_send_t
  {
    /**
     * Number of bytes sent.
     */
    size_t transferred () const noexcept
    {
      return __bits::async_send_t::transferred;
    }
  };


  /**
   * Start asynchronous async_send()
   * \see send()
   */
  void async_send (io_ptr &&io, socket_base_t::message_flags_t flags) noexcept
  {
    __bits::async_send_t::start(io.release(), base_t::socket_, flags);
  }


  /**
   * Start asynchronous async_send()
   * \see send()
   */
  void async_send (io_ptr &&io) noexcept
  {
    async_send(std::move(io), socket_base_t::message_flags_t{});
  }


  /**
   * Extract and return pointer to async_send() result from \a io
   * handle or nullptr if \a io does not represent async_send()
   * operation.
   * If asynchronous send has failed, set \a error.
   */
  static const async_send_t *async_send_result (
    const io_ptr &io, std::error_code &error) noexcept
  {
    return static_cast<async_send_t *>(
      __bits::async_send_t::result(io.get(), error)
    );
  }


  /**
   * Extract and return pointer to async_send() result from \a io
   * handle or nullptr if \a io does not represent async_send()
   * operation.
   * If asynchronous send has failed, throw std::system_error
   */
  static const async_send_t *async_send_result (const io_ptr &io)
  {
    return async_send_result(io,
      throw_on_error("basic_stream_socket::async_send")
    );
  }
};


} // namespace net


__sal_end
