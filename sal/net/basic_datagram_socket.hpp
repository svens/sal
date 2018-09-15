#pragma once

/**
 * \file sal/net/basic_datagram_socket.hpp
 * Datagram socket
 */


#include <sal/config.hpp>
#include <sal/memory.hpp>
#include <sal/net/basic_socket.hpp>
#include <sal/net/async/io.hpp>


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
  using handle_t = typename base_t::handle_t;

  /// Socket's protocol.
  using protocol_t = typename base_t::protocol_t;

  /// Socket's endpoint
  using endpoint_t = typename base_t::endpoint_t;


  basic_datagram_socket_t () = default;


  /**
   * Initialise base class using \a protocol
   */
  basic_datagram_socket_t (const Protocol &protocol)
    : base_t(protocol)
  {}


  /**
   * Initialise base class using \a endpoint
   */
  basic_datagram_socket_t (const typename Protocol::endpoint_t &endpoint)
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
  template <typename Data>
  size_t receive_from (Data &&buf,
    endpoint_t &endpoint,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    using std::begin;
    using std::end;
    auto first = begin(buf);
    auto buf_size = range_size(first, end(buf));
    auto endpoint_size = endpoint.capacity();
    auto size = base_t::socket_.receive_from(to_ptr(first), buf_size,
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
  template <typename Data>
  size_t receive_from (Data &&buf,
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
  template <typename Data>
  size_t receive_from (Data &&buf, endpoint_t &endpoint, std::error_code &error)
    noexcept
  {
    return receive_from(buf, endpoint, socket_base_t::message_flags_t{}, error);
  }


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received and stores sender address into \a endpoint. On failure,
   * throw std::system_error.
   */
  template <typename Data>
  size_t receive_from (Data &&buf, endpoint_t &endpoint)
  {
    return receive_from(buf, endpoint,
      throw_on_error("basic_datagram_socket::receive_from")
    );
  }


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received. On failure, set \a error and return 0.
   */
  template <typename Data>
  size_t receive (Data &&buf,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    using std::begin;
    using std::end;
    auto first = begin(buf);
    auto buf_size = range_size(first, end(buf));
    size_t endpoint_size = 0;
    return base_t::socket_.receive_from(to_ptr(first), buf_size,
      nullptr, &endpoint_size,
      static_cast<int>(flags),
      error
    );
  }


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received. On failure, throw std::system_error
   */
  template <typename Data>
  size_t receive (Data &&buf, socket_base_t::message_flags_t flags)
  {
    return receive(buf, flags, throw_on_error("basic_datagram_socket::receive"));
  }


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received. On failure, set \a error and return 0.
   */
  template <typename Data>
  size_t receive (Data &&buf, std::error_code &error) noexcept
  {
    return receive(buf, socket_base_t::message_flags_t{}, error);
  }


  /**
   * Receive data from this socket into \a buf. On success, returns number of
   * bytes received. On failure, throw std::system_error
   */
  template <typename Data>
  size_t receive (Data &&buf)
  {
    return receive(buf, throw_on_error("basic_datagram_socket::receive"));
  }


  /**
   * Write data of \a buf into this socket for delivering to \a endpoint. On
   * success, returns number of bytes sent. On failure, set \a error and
   * return 0.
   */
  template <typename Data>
  size_t send_to (const Data &buf,
    const endpoint_t &endpoint,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    using std::cbegin;
    using std::cend;
    auto first = cbegin(buf);
    auto buf_size = range_size(first, cend(buf));
    return base_t::socket_.send_to(to_ptr(first), buf_size,
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
  template <typename Data>
  size_t send_to (const Data &buf,
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
  template <typename Data>
  size_t send_to (const Data &buf,
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
  template <typename Data>
  size_t send_to (const Data &buf, const endpoint_t &endpoint)
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
  template <typename Data>
  size_t send (const Data &buf,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    using std::cbegin;
    using std::cend;
    auto first = cbegin(buf);
    auto buf_size = range_size(first, cend(buf));
    return base_t::socket_.send_to(to_ptr(first), buf_size,
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
  template <typename Data>
  size_t send (const Data &buf, socket_base_t::message_flags_t flags)
  {
    return send(buf, flags, throw_on_error("basic_datagram_socket::send"));
  }


  /**
   * Write data of \a buf into this socket for delivering to connected
   * endpoint. On success, returns number of bytes sent. On failure, set
   * \a error and return 0.
   */
  template <typename Data>
  size_t send (const Data &buf, std::error_code &error) noexcept
  {
    return send(buf, socket_base_t::message_flags_t{}, error);
  }


  /**
   * Write data of \a buf into this socket for delivering to connected
   * endpoint. On success, returns number of bytes sent. On failure, throw
   * std::system_error
   */
  template <typename Data>
  size_t send (const Data &buf)
  {
    return send(buf, throw_on_error("basic_datagram_socket::send"));
  }


  //
  // Asynchronous API
  //

  struct receive_from_t
  {
    size_t transferred;
    endpoint_t remote_endpoint;
    socket_base_t::message_flags_t flags;
  };


  void receive_from_async (async::io_t &&io, socket_base_t::message_flags_t flags)
    noexcept
  {
    receive_from_t *result;
    auto op = io.to_async_op(&result);
    result->flags = flags;
    base_t::async_->start_receive_from(op,
      result->remote_endpoint.data(),
      result->remote_endpoint.capacity(),
      &result->transferred,
      &result->flags
    );
  }


  void receive_from_async (async::io_t &&io) noexcept
  {
    receive_from_async(std::move(io), {});
  }


  struct receive_t
  {
    size_t transferred;
    socket_base_t::message_flags_t flags;
  };


  void receive_async (async::io_t &&io, socket_base_t::message_flags_t flags)
    noexcept
  {
    receive_t *result;
    auto op = io.to_async_op(&result);
    result->flags = flags;
    base_t::async_->start_receive(op, &result->transferred, &result->flags);
  }


  void receive_async (async::io_t &&io) noexcept
  {
    receive_async(std::move(io), {});
  }


  struct send_to_t
  {
    size_t transferred;
  };


  void send_to_async (async::io_t &&io,
    const endpoint_t &remote_endpoint,
    socket_base_t::message_flags_t flags) noexcept
  {
    send_to_t *result;
    auto op = io.to_async_op(&result);
    base_t::async_->start_send_to(op,
      remote_endpoint.data(),
      remote_endpoint.size(),
      &result->transferred,
      flags
    );
  }


  void send_to_async (async::io_t &&io, const endpoint_t &remote_endpoint)
    noexcept
  {
    send_to_async(std::move(io), remote_endpoint, {});
  }


  struct send_t
  {
    size_t transferred;
  };


  void send_async (async::io_t &&io, socket_base_t::message_flags_t flags)
    noexcept
  {
    send_t *result;
    auto op = io.to_async_op(&result);
    base_t::async_->start_send(op, &result->transferred, flags);
  }


  void send_async (async::io_t &&io) noexcept
  {
    send_async(std::move(io), {});
  }
};


template <typename Endpoint>
basic_datagram_socket_t (const Endpoint &)
  -> basic_datagram_socket_t<typename Endpoint::protocol_t>;


} // namespace net


__sal_end
