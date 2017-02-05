#pragma once

/**
 * \file sal/net/basic_stream_socket.hpp
 * Stream socket
 */


#include <sal/config.hpp>
#include <sal/buf_ptr.hpp>
#include <sal/net/basic_socket.hpp>
#include <sal/net/io_buf.hpp>
#include <sal/net/io_context.hpp>


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
   * Initialise base class using \a handle
   */
  basic_stream_socket_t (const native_handle_t &handle)
    : base_t(handle)
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
    return base_t::impl_.receive(buf.data(), buf.size(),
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
    return base_t::impl_.send(buf.data(), buf.size(),
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


#if __sal_os_windows


  //
  // Asynchronous API
  //


  struct async_receive_t
    : public __bits::async_receive_t
  {
    size_t transferred () const noexcept
    {
      return this->transferred_;
    }
  };


  void async_receive (io_buf_ptr &&io_buf,
    socket_base_t::message_flags_t flags) noexcept
  {
    auto &request = *io_buf->make_request<async_receive_t>();
    auto completed = base_t::impl_.start(io_buf.get(),
      io_buf->data(), io_buf->size(),
      flags,
      request
    );
    if (completed)
    {
      io_context_t::notify(io_buf.get());
    }
    io_buf.release();
  }


  void async_receive (io_buf_ptr &&io_buf) noexcept
  {
    async_receive(std::move(io_buf), socket_base_t::message_flags_t{});
  }


  static const async_receive_t *async_receive_result (const io_buf_ptr &io_buf,
    std::error_code &error) noexcept
  {
    if (auto result = io_buf->make_result<async_receive_t>())
    {
      if (result->error_)
      {
        error = result->error_;
      }
      return result;
    }
    return nullptr;
  }


  static const async_receive_t *async_receive_result (const io_buf_ptr &io_buf)
  {
    return async_receive_result(io_buf,
      throw_on_error("basic_stream_socket::async_receive")
    );
  }


  struct async_send_t
    : public __bits::async_send_t
  {
    size_t transferred () const noexcept
    {
      return this->transferred_;
    }
  };


  void async_send (io_buf_ptr &&io_buf,
    socket_base_t::message_flags_t flags) noexcept
  {
    auto &request = *io_buf->make_request<async_send_t>();
    auto completed = base_t::impl_.start(io_buf.get(),
      io_buf->data(), io_buf->size(),
      flags,
      request
    );
    if (completed)
    {
      io_context_t::notify(io_buf.get());
    }
    io_buf.release();
  }


  void async_send (io_buf_ptr &&io_buf) noexcept
  {
    async_send(std::move(io_buf), socket_base_t::message_flags_t{});
  }


  static const async_send_t *async_send_result (const io_buf_ptr &io_buf,
    std::error_code &error) noexcept
  {
    if (auto result = io_buf->make_result<async_send_t>())
    {
      if (result->error_)
      {
        error = result->error_;
      }
      return result;
    }
    return nullptr;
  }


  static const async_send_t *async_send_result (const io_buf_ptr &io_buf)
  {
    return async_send_result(io_buf,
      throw_on_error("basic_stream_socket::async_send")
    );
  }


#endif // __sal_os_windows
};


} // namespace net


__sal_end
