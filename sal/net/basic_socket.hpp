#pragma once

/**
 * \file sal/net/basic_socket.hpp
 * Datagram and stream sockets' base class
 */


#include <sal/config.hpp>
#include <sal/net/error.hpp>
#include <sal/net/socket_base.hpp>
#include <sal/net/socket_options.hpp>


__sal_begin


namespace net {


/**
 * Base class for basic_datagram_socket<Protocol> and
 * basic_stream_socket<Protocol>. It provides functionality that is common to
 * both types of socket.
 */
template <typename Protocol>
class basic_socket_t
  : public socket_base_t
{
public:

  /**
   * Socket's protocol.
   */
  using protocol_t = Protocol;

  /**
   * Socket's endpoint
   */
  using endpoint_t = typename Protocol::endpoint_t;


  /**
   * Return native representation of this socket.
   */
  native_handle_t native_handle () const noexcept
  {
    return handle_;
  }


  /**
   * Return boolean indicating whether this socket was opened by previous call
   * to open() or assign().
   */
  bool is_open () const noexcept
  {
    return handle_ != invalid_socket;
  }


  /**
   * Create new socket instance of \a protocol. On failure, set \a error
   */
  void open (const protocol_t &protocol, std::error_code &error) noexcept
  {
    if (!is_open())
    {
      handle_ = __bits::open(protocol.family(),
        protocol.type(),
        protocol.protocol(),
        error
      );
    }
    else
    {
      error = make_error_code(socket_errc_t::already_open);
    }
  }


  /**
   * Create new socket instance of \a protocol. On failure, throw
   * std::system_error
   */
  void open (const protocol_t &protocol)
  {
    open(protocol, throw_on_error("basic_socket::open"));
  }


  /**
   * Assign previously opened native socket \a handle to this socket object.
   * On failure, set \a error.
   */
  void assign (const native_handle_t &handle, std::error_code &error) noexcept
  {
    if (handle == invalid_socket)
    {
      error = make_error_code(std::errc::bad_file_descriptor);
    }
    else if (!is_open())
    {
      handle_ = handle;
    }
    else
    {
      error = make_error_code(socket_errc_t::already_open);
    }
  }


  /**
   * Assign previously opened native socket \a handle to this socket object.
   * On failure, throw std::system_error
   */
  void assign (const native_handle_t &handle)
  {
    assign(handle, throw_on_error("basic_socket::assign"));
  }


  /**
   * Close socket, releasing all internal resources. On failure, set \a error.
   */
  void close (std::error_code &error) noexcept
  {
    if (is_open())
    {
      __bits::close(handle_, error);
      handle_ = invalid_socket;
    }
    else
    {
      error = make_error_code(std::errc::bad_file_descriptor);
    }
  }


  /**
   * Close socket, releasing all internal resources. On failure, throw
   * std::system_error
   */
  void close ()
  {
    close(throw_on_error("basic_socket::close"));
  }


  /**
   * Get socket \a option. On failure, set \a error
   */
  template <typename GettableSocketOption>
  void get_option (const GettableSocketOption &option, std::error_code &error)
    const noexcept
  {
    typename GettableSocketOption::native_t data{};
    socklen_t size = sizeof(data);
    __bits::get_opt(handle_,
      option.level, option.name,
      &data, &size,
      error
    );
    if (!error)
    {
      option.load(data, size);
    }
  }


  /**
   * Get socket \a option. On failure, throw std::system_error
   */
  template <typename GettableSocketOption>
  void get_option (const GettableSocketOption &option) const
  {
    get_option(option, throw_on_error("basic_socket::get_option"));
  }


  /**
   * Set socket \a option. On failure, set \a error
   */
  template <typename SettableSocketOption>
  void set_option (const SettableSocketOption &option,
    std::error_code &error) noexcept
  {
    typename SettableSocketOption::native_t data;
    option.store(data);
    __bits::set_opt(handle_,
      option.level, option.name,
      &data, sizeof(data),
      error
    );
  }


  /**
   * Set socket \a option. On failure, throw std::system_error
   */
  template <typename SettableSocketOption>
  void set_option (const SettableSocketOption &option)
  {
    set_option(option, throw_on_error("basic_socket::set_option"));
  }


  /**
   * Set socket to non-blocking \a mode. On failure, set \a error
   */
  void non_blocking (bool mode, std::error_code &error) noexcept
  {
    __bits::non_blocking(handle_, mode, error);
  }


  /**
   * Set socket to non-blocking \a mode. On failure, throw std::system_error
   */
  void non_blocking (bool mode)
  {
    non_blocking(mode, throw_on_error("basic_socket::non_blocking"));
  }


  /**
   * Query socket non-blocking mode. On failure, set \a error.
   * \note This method is not supported on Windows platforms.
   */
  bool non_blocking (std::error_code &error) const noexcept
  {
    return __bits::non_blocking(handle_, error);
  }


  /**
   * Query socket non-blocking mode. On failure, throw std::system_error
   * \note This method is not supported on Windows platforms.
   */
  bool non_blocking () const
  {
    return non_blocking(throw_on_error("basic_socket::non_blocking"));
  }


  /**
   * Returns number of bytes that may be read without blocking. On failure,
   * set \a error and returned value is undefined.
   */
  size_t available (std::error_code &error) const noexcept
  {
    return __bits::available(handle_, error);
  }


  /**
   * Returns number of bytes that may be read without blocking. On failure,
   * throw std::system_error
   */
  size_t available () const
  {
    return available(throw_on_error("basic_socket::available"));
  }


  /**
   * Bind this socket to specified local \a endpoint. On failure, set \a error
   */
  void bind (const endpoint_t &endpoint, std::error_code &error) noexcept
  {
    __bits::bind(handle_, endpoint.data(), endpoint.size(), error);
  }


  /**
   * Bind this socket to specified local \a endpoint. On failure, throw
   * std::system_error
   */
  void bind (const endpoint_t &endpoint)
  {
    bind(endpoint, throw_on_error("basic_socket::bind"));
  }


  /**
   * Connect this socket to specified remote \a endpoint. If is_open() is
   * false, it is open() first. On failure, set \a error
   */
  void connect (const endpoint_t &endpoint, std::error_code &error) noexcept
  {
    if (!is_open())
    {
      open(endpoint.protocol(), error);
      // LCOV_EXCL_START
      // API prevents intentional errors for testing
      if (error)
      {
        return;
      }
      // LCOV_EXCL_STOP
    }
    __bits::connect(handle_, endpoint.data(), endpoint.size(), error);
  }


  /**
   * Connect this socket to specified remote \a endpoint. If is_open() is
   * false, it is open() first. On failure, throw std:system_error
   */
  void connect (const endpoint_t &endpoint)
  {
    connect(endpoint, throw_on_error("basic_socket::connect"));
  }


  /**
   * Shuts down all or part of a full-duplex connection for the socket
   * according to combination of flags \a what. On failure, set \a error
   */
  void shutdown (shutdown_t what, std::error_code &error) noexcept
  {
    __bits::shutdown(handle_, static_cast<int>(what), error);
  }


  /**
   * Shuts down all or part of a full-duplex connection for the socket
   * according to combination of flags \a what. On failure, throw
   * std::system_error.
   */
  void shutdown (shutdown_t what)
  {
    return shutdown(what, throw_on_error("basic_socket::shutdown"));
  }


  /**
   * Wait up to \a duration socket to be ready to read or write, depending on
   * \a what. Return true if socket become ready for desired operation, and
   * false if timeout arrived. If timeout is zero, wait will return without
   * blocking. On failure, \a set error and return false.
   */
  template <class Rep, class Period>
  bool wait (wait_t what,
    const std::chrono::duration<Rep, Period> &duration,
    std::error_code &error) noexcept
  {
    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return __bits::wait(handle_, what, static_cast<int>(d.count()), error);
  }


  /**
   * Wait up to \a duration socket to be ready to read or write, depending on
   * \a what. Return true if socket become ready for desired operation, and
   * false if timeout arrived. If timeout is zero, wait will return without
   * blocking. On failure, throw std::system_error
   */
  template <class Rep, class Period>
  bool wait (wait_t what, const std::chrono::duration<Rep, Period> &duration)
  {
    return wait(what, duration, throw_on_error("basic_socket::wait"));
  }


  /**
   * Determine the locally-bound endpoint associated with the socket. On
   * failure, set \a error and returned endpoint value is undefined.
   */
  endpoint_t local_endpoint (std::error_code &error) const noexcept
  {
    endpoint_t endpoint;
    size_t endpoint_size = endpoint.capacity();
    __bits::local_endpoint(handle_, endpoint.data(), &endpoint_size, error);
    if (!error)
    {
      endpoint.resize(endpoint_size);
    }
    return endpoint;
  }


  /**
   * Determine the locally-bound endpoint associated with the socket. On
   * failure, throw std::system_error.
   */
  endpoint_t local_endpoint () const
  {
    return local_endpoint(throw_on_error("basic_socket::local_endpoint"));
  }


  /**
   * Determine the remote endpoint associated with the socket. On
   * failure, set \a error and returned endpoint value is undefined.
   */
  endpoint_t remote_endpoint (std::error_code &error) const noexcept
  {
    endpoint_t endpoint;
    size_t endpoint_size = endpoint.capacity();
    __bits::remote_endpoint(handle_, endpoint.data(), &endpoint_size, error);
    if (!error)
    {
      endpoint.resize(endpoint_size);
    }
    return endpoint;
  }


  /**
   * Determine the remote endpoint associated with the socket. On failure,
   * throw std::system_error.
   */
  endpoint_t remote_endpoint () const
  {
    return remote_endpoint(throw_on_error("basic_socket::remote_endpoint"));
  }


protected:

  basic_socket_t () = default;


  /**
   * Construct new basic_socket_t using native_handle() from \a that. After
   * move, that.is_open() == false
   */
  basic_socket_t (basic_socket_t &&that) noexcept
    : handle_(that.handle_)
  {
    that.handle_ = invalid_socket;
  }


  /**
   * If is \a is_open(), close() socket and release socket resources. On
   * failure, errors are silently ignored.
   */
  ~basic_socket_t () noexcept
  {
    if (is_open())
    {
      std::error_code ignored;
      close(ignored);
    }
  }


  /**
   * Construct and open new socket using \a protocol. On failure, throw
   * std::system_error.
   */
  basic_socket_t (const protocol_t &protocol)
  {
    open(protocol);
  }


  /**
   * Construct new socket, open and bind to \a endpoint. On failure, throw
   * std::system_error
   */
  basic_socket_t (const endpoint_t &endpoint)
    : basic_socket_t(endpoint.protocol())
  {
    bind(endpoint);
  }


  /**
   * Construct new socket, acquiring \a handle
   */
  basic_socket_t (const native_handle_t &handle)
  {
    assign(handle);
  }


  /**
   * If this is_open(), close() it and then move all internal resource from
   * \a that to \a this.
   */
  basic_socket_t &operator= (basic_socket_t &&that) noexcept
  {
    auto tmp{std::move(*this)};
    handle_ = that.handle_;
    that.handle_ = invalid_socket;
    return *this;
  }


  basic_socket_t (const basic_socket_t &) = delete;
  basic_socket_t &operator= (const basic_socket_t &) = delete;


private:

  native_handle_t handle_ = invalid_socket;
};


} // namespace net


__sal_end
