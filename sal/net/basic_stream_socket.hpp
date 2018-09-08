#pragma once

/**
 * \file sal/net/basic_stream_socket.hpp
 * Stream socket
 */


#include <sal/config.hpp>
#include <sal/memory.hpp>
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
  using handle_t = typename base_t::handle_t;

  /// Socket's protocol.
  using protocol_t = typename base_t::protocol_t;

  /// Socket's endpoint
  using endpoint_t = typename base_t::endpoint_t;


  basic_stream_socket_t () = default;


  /**
   * Initialise base class using \a protocol
   */
  basic_stream_socket_t (const Protocol &protocol)
    : base_t(protocol)
  {}


  /**
   * Initialise base class using \a endpoint
   */
  basic_stream_socket_t (const typename Protocol::endpoint_t &endpoint)
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
  template <typename Data>
  size_t receive (Data &&buf,
    socket_base_t::message_flags_t flags,
    std::error_code &error) noexcept
  {
    using std::begin;
    using std::end;
    auto first = begin(buf);
    auto size = range_size(first, end(buf));
    return base_t::socket_.receive(to_ptr(first), size,
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
    return receive(buf, flags, throw_on_error("basic_stream_socket::receive"));
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
    return receive(buf, throw_on_error("basic_stream_socket::receive"));
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
    auto size = range_size(first, cend(buf));
    return base_t::socket_.send(to_ptr(first), size,
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
    return send(buf, flags, throw_on_error("basic_stream_socket::send"));
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
    return send(buf, throw_on_error("basic_stream_socket::send"));
  }
};


template <typename Endpoint>
basic_stream_socket_t (const Endpoint &)
  -> basic_stream_socket_t<typename Endpoint::protocol_t>;


} // namespace net


__sal_end
