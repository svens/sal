#pragma once

/**
 * \file sal/net/basic_socket_acceptor.hpp
 * Socket acceptor
 */


#include <sal/config.hpp>
#include <sal/net/error.hpp>
#include <sal/net/socket_base.hpp>
#include <sal/net/socket_options.hpp>


__sal_begin


namespace net {


/**
 * Object of class basic_socket_acceptor_t is used to listen and queue
 * incoming socket connections. Socket object that represent incoming
 * connections are dequeued by calling accept().
 *
 * For more information about asynchronous API usage, see async_service_t.
 */
template <typename AcceptableProtocol>
class basic_socket_acceptor_t
  : public socket_base_t
{
public:

  /// Native acceptor handle type
  using handle_t = socket_base_t::handle_t;

  /// Acceptable protocol
  using protocol_t = AcceptableProtocol;

  /// Acceptable endpoint
  using endpoint_t = typename protocol_t::endpoint_t;

  /// Acceptable socket
  using socket_t = typename protocol_t::socket_t;


  basic_socket_acceptor_t () = default;


  /**
   * Construct and open new acceptor using \a protocol. On failure, throw
   * std::system_error.
   */
  basic_socket_acceptor_t (const protocol_t &protocol)
  {
    open(protocol);
  }


  /**
   * Construct new acceptor, open and bind to \a endpoint. On failure, throw
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
   * Construct new acceptor for pre-opened \a handle using \a protocol.
   * On failure, throw std::system_error
   */
  basic_socket_acceptor_t (const protocol_t &protocol, handle_t handle)
  {
    assign(protocol, handle);
  }


  /**
   * Return native representation of this socket.
   */
  handle_t native_handle () const noexcept
  {
    return socket_.handle;
  }


  /**
   * Return boolean indicating whether this socket was opened by previous call
   * to open() or assign().
   */
  bool is_open () const noexcept
  {
    return socket_.handle != invalid;
  }


  /**
   * Create new socket instance of \a protocol. On failure, set \a error
   */
  void open (const protocol_t &protocol, std::error_code &error) noexcept
  {
    if (!is_open())
    {
      family_ = protocol.family();
      socket_.open(family_, protocol.type(), protocol.protocol(), error);
    }
    else
    {
      error = make_error_code(socket_errc::already_open);
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
   * Assign previously opened native socket \a handle (using \a protocol) to
   * this socket object. On failure, set \a error.
   */
  void assign (const protocol_t &protocol, handle_t handle,
    std::error_code &error) noexcept
  {
    if (handle == invalid)
    {
      error = make_error_code(std::errc::bad_file_descriptor);
    }
    else if (!is_open())
    {
      family_ = protocol.family();
      socket_.handle = handle;
    }
    else
    {
      error = make_error_code(socket_errc::already_open);
    }
  }


  /**
   * Assign previously opened native socket \a handle (using \a protocol) to
   * this socket object. On failure, throw std::system_error
   */
  void assign (const protocol_t &protocol, handle_t handle)
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
      socket_.close(error);
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
    auto size = sizeof(data);
    socket_.get_opt(option.level, option.name, &data, &size, error);
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
    socket_.set_opt(option.level, option.name, &data, sizeof(data), error);
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
    socket_.non_blocking(mode, error);
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
    return socket_.non_blocking(error);
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
    socket_.bind(endpoint.data(), endpoint.size(), error);
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
    socket_.listen(backlog, error);
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
    auto h = socket_.accept(endpoint.data(), &endpoint_size,
      enable_connection_aborted_,
      error
    );
    if (!error)
    {
      endpoint.resize(endpoint_size);
      return h;
    }
    return socket_t{};
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
    auto h = socket_.accept(nullptr, nullptr, enable_connection_aborted_, error);
    if (!error)
    {
      return h;
    }
    return socket_t{};
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
    return socket_.wait(what, static_cast<int>(d.count()), error);
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
    auto endpoint_size = endpoint.capacity();
    socket_.local_endpoint(endpoint.data(), &endpoint_size, error);
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
    socket_base_t::associate(socket_, service, error);
  }


  /**
   * \see associate (async_service_t &, std::error_code &)
   * \throws std::system_error on failure
   */
  void associate (async_service_t &service)
  {
    associate(service, throw_on_error("basic_socket_acceptor::associate"));
  }


  /**
   * async_accept() result.
   */
  struct async_accept_t
    : public __bits::async_accept_t
  {
    /**
     * Remote endpoint of accepted socket.
     */
    const endpoint_t &remote_endpoint () const noexcept
    {
      return *reinterpret_cast<const endpoint_t *>(
        __bits::async_accept_t::remote_address
      );
    }

    /**
     * Return accepted socket.
     *
     * \note Only first call returns valid accepted handle. Following calls
     * return closed socket.
     */
    socket_t accepted () noexcept
    {
      auto handle = __bits::async_accept_t::load_accepted();
      return handle != socket_t::invalid ? handle : socket_t{};
    }
  };


  /**
   * Start asynchronous async_accept()
   * \see accept()
   */
  void async_accept (io_ptr &&io) noexcept
  {
    __bits::async_accept_t::start(io.release(), socket_, family_);
  }


  /**
   * Extract and return pointer to async_accept() result from \a io
   * handle or nullptr if \a io does not represent async_accept()
   * operation.
   * If asynchronous accept has failed, set \a error.
   */
  static async_accept_t *async_accept_result (
    const io_ptr &io, std::error_code &error) noexcept
  {
    return static_cast<async_accept_t *>(
      __bits::async_accept_t::result(io.get(), error)
    );
  }


  /**
   * Extract and return pointer to async_accept() result from \a io
   * handle or nullptr if \a io does not represent async_accept()
   * operation.
   * If asynchronous accept has failed, throw std::system_error
   */
  static async_accept_t *async_accept_result (const io_ptr &io)
  {
    return async_accept_result(io,
      throw_on_error("basic_socket_acceptor::async_accept")
    );
  }


private:

  __bits::socket_t socket_;
  int family_ = AF_UNSPEC;
  bool enable_connection_aborted_ = false;
};


} // namespace net


__sal_end
