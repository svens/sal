#pragma once

/**
 * \file sal/net/basic_socket_acceptor.hpp
 * Socket acceptor
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/error.hpp>
#include <sal/net/socket_base.hpp>
#include <sal/net/socket_options.hpp>


__sal_begin


namespace net {


/**
 */
template <typename AcceptableProtocol>
class basic_socket_acceptor_t
  : public socket_base_t
{
public:

  /// Native acceptor handle type
  using native_handle_t = socket_base_t::native_handle_t;

  /// Acceptable protocol
  using protocol_t = AcceptableProtocol;

  /// Acceptable endpoint
  using endpoint_t = typename protocol_t::endpoint_t;

  /// Acceptable socket
  using socket_t = typename protocol_t::socket_t;

  basic_socket_acceptor_t () = default;


  /**
   * Construct new basic_socket_acceptor_t using native_handle() from \a that. After
   * move, that.is_open() == false
   */
  basic_socket_acceptor_t (basic_socket_acceptor_t &&that) noexcept
    : handle_(that.handle_)
  {
    that.handle_ = invalid_socket;
  }


  /**
   * If is \a is_open(), close() socket and release socket resources. On
   * failure, errors are silently ignored.
   */
  ~basic_socket_acceptor_t () noexcept
  {
    if (is_open())
    {
      std::error_code ignored;
      close(ignored);
    }
  }


  /**
   * Construct and open new acceptor using \a protocol. On failure, throw
   * std::system_error.
   */
  basic_socket_acceptor_t (const protocol_t &protocol)
  {
    open(protocol);
  }


  /**
   * Construct new socket, open and bind to \a endpoint. On failure, throw
   * std::system_error
   */
  basic_socket_acceptor_t (const endpoint_t &endpoint, bool reuse_addr=true)
    : basic_socket_acceptor_t(endpoint.protocol())
  {
    if (reuse_addr)
    {
      set_option(reuse_address(reuse_addr));
    }
    bind(endpoint);
    listen();
  }


  /**
   * Construct new socket, open() with \a protocol and bind() to \a endpoint.
   * On failure, throw std::system_error
   */
  basic_socket_acceptor_t (const protocol_t &protocol, const native_handle_t &handle)
  {
    assign(protocol, handle);
  }


  /**
   * If this is_open(), close() it and then move all internal resource from
   * \a that to \a this.
   */
  basic_socket_acceptor_t &operator= (basic_socket_acceptor_t &&that) noexcept
  {
    auto tmp{std::move(*this)};
    handle_ = that.handle_;
    that.handle_ = invalid_socket;
    return *this;
  }


  basic_socket_acceptor_t (const basic_socket_acceptor_t &) = delete;
  basic_socket_acceptor_t &operator= (const basic_socket_acceptor_t &) = delete;


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
    open(protocol, throw_on_error("basic_socket_acceptor::open"));
  }


  /**
   * Assign previously opened native socket \a handle to this socket object.
   * On failure, set \a error.
   */
  void assign (const protocol_t &,
    const native_handle_t &handle,
    std::error_code &error) noexcept
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
  void assign (const protocol_t &protocol, const native_handle_t &handle)
  {
    assign(protocol, handle, throw_on_error("basic_socket_acceptor::assign"));
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
    close(throw_on_error("basic_socket_acceptor::close"));
  }


  /**
   * Get socket \a option. On failure, set \a error
   */
  template <typename GettableSocketOption>
  void get_option (const GettableSocketOption &option, std::error_code &error)
    const noexcept
  {
    typename GettableSocketOption::native_t data;
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
    get_option(option, throw_on_error("basic_socket_acceptor::get_option"));
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
    set_option(option, throw_on_error("basic_socket_acceptor::set_option"));
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
    non_blocking(mode, throw_on_error("basic_socket_acceptor::non_blocking"));
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
    return non_blocking(throw_on_error("basic_socket_acceptor::non_blocking"));
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
    bind(endpoint, throw_on_error("basic_socket_acceptor::bind"));
  }


  /**
   * Marks this acceptor as ready to accept connections. On failure, set
   * \a error
   */
  void listen (int backlog, std::error_code &error) noexcept
  {
    __bits::listen(handle_, backlog, error);
  }


  /**
   * Marks this acceptor as ready to accept connections. On failure, throw
   * std::system_error
   */
  void listen (int backlog = socket_base_t::max_listen_connections)
  {
    listen(backlog, throw_on_error("basic_socket_acceptor::listen"));
  }


  /**
   * Extracts a socket from the queue of pending connections. Assign accepted
   * socket remote address to \a endpoint. On error, set \a error and return
   * socket_t()
   */
  socket_t accept (endpoint_t &endpoint, std::error_code &error) noexcept
  {
    auto endpoint_size = endpoint.capacity();
    auto h = __bits::accept(handle_,
      endpoint.data(), &endpoint_size,
      enable_connection_aborted_,
      error
    );
    if (!error)
    {
      endpoint.resize(endpoint_size);
    }
    // TODO: protocol
    return socket_t{protocol_t::v4(), h};
  }


  /**
   * Extracts a socket from the queue of pending connections. Assign accepted
   * socket remote address to \a endpoint. On error, throw std::system_error
   */
  socket_t accept (endpoint_t &endpoint)
  {
    return accept(endpoint, throw_on_error("basic_socket_acceptor::accept"));
  }


  /**
   * Extracts a socket from the queue of pending connections. On error, set
   * \a error
   */
  socket_t accept (std::error_code &error) noexcept
  {
    auto h = __bits::accept(handle_,
      nullptr, 0,
      enable_connection_aborted_,
      error
    );
  }


  /**
   * Extracts a socket from the queue of pending connections. On error, throw
   * std::system_error.
   */
  socket_t accept ()
  {
    return accept(throw_on_error("basic_socket_acceptor::accept"));
  }


  /**
   * If \a mode is true, subsequent accept operations on this acceptor are
   * permitted to fail with error condition std::errc::connection_aborted. If
   * \a mode is false, subsequent accept operations will not fail with
   * std::errc::connection_aborted but will restart accept operation instead.
   */
  void enable_connection_aborted (bool mode) noexcept
  {
    enable_connection_aborted_ = mode;
  }


  /**
   * Returns whether accept operations on this acceptor are permitted to fail
   * with std::errc::connection_aborted.
   */
  bool enable_connection_aborted () const noexcept
  {
    return enable_connection_aborted_;
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
    return wait(what, duration, throw_on_error("basic_socket_acceptor::wait"));
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
    return local_endpoint(throw_on_error("basic_socket_acceptor::local_endpoint"));
  }


private:

  native_handle_t handle_ = invalid_socket;
  bool enable_connection_aborted_ = false;
};


} // namespace net


__sal_end
