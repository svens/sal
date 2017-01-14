#pragma once

/**
 * \file sal/net/basic_datagram_socket.hpp
 * Datagram socket
 */


#include <sal/config.hpp>
#include <sal/net/basic_socket.hpp>
#include <sal/buf_ptr.hpp>


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
   * Initialise base class using \a handle
   */
  basic_datagram_socket_t (const native_handle_t &handle)
    : base_t(handle)
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
    auto size = base_t::impl_.recv_from(buf.data(), buf.size(),
      static_cast<int>(flags),
      endpoint.data(), &endpoint_size,
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
    return base_t::impl_.recv_from(buf.data(), buf.size(),
      static_cast<int>(flags),
      nullptr, &endpoint_size,
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
    return base_t::impl_.send_to(buf.data(), buf.size(),
      static_cast<int>(flags),
      endpoint.data(), endpoint.size(),
      error
    );
  }


  /**
   * Write daa of \a buf into this socket for delivering to \a endpoint. On
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
    return base_t::impl_.send_to(buf.data(), buf.size(),
      static_cast<int>(flags),
      nullptr, 0,
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
};


} // namespace net


__sal_end
