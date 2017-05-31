#pragma once

/**
 * \file sal/net/basic_datagram_socket.hpp
 * Datagram socket
 */


#include <sal/config.hpp>
#include <sal/buf_ptr.hpp>
#include <sal/net/basic_socket.hpp>


__sal_begin


namespace net {


/**
 * Datagram socket
 *
 * For more information about asynchronous API usage, see async_service_t.
 */
template <typename Protocol>
class basic_datagram_socket_t
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


  basic_datagram_socket_t () = default;


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
   * Initialise base class using \a handle
   */
  basic_datagram_socket_t (handle_t handle)
    : base_t(handle)
  {}


  //
  // Synchronous API
  //


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received and stores sender address into \a endpoint. On failure,
   * set \a error and return 0.
   */
  template <typename Ptr>
  size_t receive_from (const Ptr &buf,
    endpoint_t &endpoint,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    auto endpoint_size = endpoint.capacity();
    auto size = base_t::socket_.receive_from(buf.data(), buf.size(),
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
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received and stores sender address into \a endpoint. On failure,
   * throw std::system_error.
   */
  template <typename Ptr>
  size_t receive_from (const Ptr &buf,
    endpoint_t &endpoint,
    socket_base_t::message_flags_t flags)
  {
    return receive_from(buf, endpoint, flags,
      throw_on_error("basic_datagram_socket::receive_from")
    );
  }


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received and stores sender address into \a endpoint. On failure,
   * set \a error and return 0.
   */
  template <typename Ptr>
  size_t receive_from (const Ptr &buf,
    endpoint_t &endpoint,
    std::error_code &error) noexcept
  {
    return receive_from(buf, endpoint, socket_base_t::message_flags_t{}, error);
  }


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received and stores sender address into \a endpoint. On failure,
   * throw std::system_error.
   */
  template <typename Ptr>
  size_t receive_from (const Ptr &buf, endpoint_t &endpoint)
  {
    return receive_from(buf, endpoint,
      throw_on_error("basic_datagram_socket::receive_from")
    );
  }


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received. On failure, set \a error and return 0.
   */
  template <typename Ptr>
  size_t receive (const Ptr &buf,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    size_t endpoint_size = 0;
    return base_t::socket_.receive_from(buf.data(), buf.size(),
      nullptr, &endpoint_size,
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
    return receive(buf, flags, throw_on_error("basic_datagram_socket::receive"));
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
    return receive(buf, throw_on_error("basic_datagram_socket::receive"));
  }


  /**
   * Write data of \a buf into this socket for delivering to \a endpoint. On
   * success, returns number of bytes sent. On failure, set \a error and
   * return 0.
   */
  template <typename Ptr>
  size_t send_to (const Ptr &buf,
    const endpoint_t &endpoint,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    return base_t::socket_.send_to(buf.data(), buf.size(),
      endpoint.data(), endpoint.size(),
      static_cast<int>(flags),
      error
    );
  }


  /**
   * Write data of \a buf into this socket for delivering to \a endpoint. On
   * success, returns number of bytes sent. On failure, throw
   * std::system_error
   */
  template <typename Ptr>
  size_t send_to (const Ptr &buf,
    const endpoint_t &endpoint,
    socket_base_t::message_flags_t flags)
  {
    return send_to(buf, endpoint, flags,
      throw_on_error("basic_datagram_socket::send_to")
    );
  }


  /**
   * Write data of \a buf into this socket for delivering to \a endpoint. On
   * success, returns number of bytes sent. On failure, set \a error and
   * return 0.
   */
  template <typename Ptr>
  size_t send_to (const Ptr &buf,
    const endpoint_t &endpoint,
    std::error_code &error) noexcept
  {
    return send_to(buf, endpoint, socket_base_t::message_flags_t{}, error);
  }


  /**
   * Write data of \a buf into this socket for delivering to \a endpoint. On
   * success, returns number of bytes sent. On failure, throw
   * std::system_error
   */
  template <typename Ptr>
  size_t send_to (const Ptr &buf, const endpoint_t &endpoint)
  {
    return send_to(buf, endpoint,
      throw_on_error("basic_datagram_socket::send_to")
    );
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
    return base_t::socket_.send_to(buf.data(), buf.size(),
      nullptr, 0,
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
    return send(buf, flags, throw_on_error("basic_datagram_socket::send"));
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
    return send(buf, throw_on_error("basic_datagram_socket::send"));
  }


  //
  // Asynchronous API
  //

  /**
   * Associate this socket with \a service for asynchronous I/O operations.
   * Using asynchronous API without associating it first with service is
   * undefined behaviour. Once socket is associated with specific service, it
   * will remain so until closed.
   *
   * On failure, set \a error
   */
  void associate (async_service_t &service, std::error_code &error) noexcept
  {
    socket_base_t::associate(base_t::socket_, service, error);
  }


  /**
   * \see associate (async_service_t &, std::error_code &)
   * \throws std::system_error on failure
   */
  void associate (async_service_t &service)
  {
    associate(service, throw_on_error("basic_datagram_socket::associate"));
  }


  /**
   * async_receive_from() result.
   */
  struct async_receive_from_t
    : public __bits::async_receive_from_t
  {
    /**
     * Packet sender endpoint.
     */
    const endpoint_t &endpoint () const noexcept
    {
      return *reinterpret_cast<const endpoint_t *>(&address);
    }

    /**
     * Number of bytes received.
     */
    size_t transferred () const noexcept
    {
      return __bits::async_receive_from_t::transferred;
    }
  };


  /**
   * Start asynchronous async_receive_from()
   * \see receive_from()
   */
  void async_receive_from (io_ptr &&io, socket_base_t::message_flags_t flags)
    noexcept
  {
    __bits::async_receive_from_t::start(io.release(), base_t::socket_, flags);
  }


  /**
   * Start asynchronous async_receive_from()
   * \see receive_from()
   */
  void async_receive_from (io_ptr &&io) noexcept
  {
    async_receive_from(std::move(io), socket_base_t::message_flags_t{});
  }


  /**
   * Extract and return pointer to async_receive_from() result from \a io
   * handle or nullptr if \a io does not represent async_receive_from()
   * operation.
   * If asynchronous receive has failed, set \a error.
   */
  static const async_receive_from_t *async_receive_from_result (
    const io_ptr &io, std::error_code &error) noexcept
  {
    return static_cast<async_receive_from_t *>(
      __bits::async_receive_from_t::result(io.get(), error)
    );
  }


  /**
   * Extract and return pointer to async_receive_from() result from \a io
   * handle or nullptr if \a io does not represent async_receive_from()
   * operation.
   * If asynchronous receive has failed, throw std::system_error
   */
  static const async_receive_from_t *async_receive_from_result (const io_ptr &io)
  {
    return async_receive_from_result(io,
      throw_on_error("basic_datagram_socket::async_receive_from")
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
      throw_on_error("basic_datagram_socket::async_receive")
    );
  }


  /**
   * async_send_to() result.
   */
  struct async_send_to_t
    : public __bits::async_send_to_t
  {
    /**
     * Number of bytes sent.
     */
    size_t transferred () const noexcept
    {
      return __bits::async_send_to_t::transferred;
    }
  };


  /**
   * Start asynchronous async_send_to()
   * \see send_to()
   */
  void async_send_to (io_ptr &&io,
    const endpoint_t &endpoint,
    socket_base_t::message_flags_t flags) noexcept
  {
    __bits::async_send_to_t::start(io.release(),
      base_t::socket_,
      endpoint.data(), endpoint.size(),
      flags
    );
  }


  /**
   * Start asynchronous async_send_to()
   * \see send_to()
   */
  void async_send_to (io_ptr &&io, const endpoint_t &endpoint) noexcept
  {
    async_send_to(std::move(io), endpoint, socket_base_t::message_flags_t{});
  }


  /**
   * Extract and return pointer to async_send_to() result from \a io
   * handle or nullptr if \a io does not represent async_send_to()
   * operation.
   * If asynchronous send has failed, set \a error.
   */
  static const async_send_to_t *async_send_to_result (
    const io_ptr &io, std::error_code &error) noexcept
  {
    return static_cast<async_send_to_t *>(
      __bits::async_send_to_t::result(io.get(), error)
    );
  }


  /**
   * Extract and return pointer to async_send_to() result from \a io
   * handle or nullptr if \a io does not represent async_send_to()
   * operation.
   * If asynchronous send has failed, throw std::system_error
   */
  static const async_send_to_t *async_send_to_result (const io_ptr &io)
  {
    return async_send_to_result(io,
      throw_on_error("basic_datagram_socket::async_send_to")
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
      throw_on_error("basic_datagram_socket::async_send")
    );
  }
};


} // namespace net


__sal_end
