#pragma once

/**
 * \file sal/net/basic_socket.hpp
 * Datagram and stream sockets' base class
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/error.hpp>
#include <sal/net/socket_base.hpp>
#include <sal/net/socket_options.hpp>


__sal_begin


namespace net {


/**
 * Datagram and stream sockets' base class
 */
template <typename Protocol>
class basic_socket_t
  : public socket_base_t
{
public:

  using protocol_t = Protocol;
  using endpoint_t = typename Protocol::endpoint_t;


  native_handle_t native_handle () const noexcept
  {
    return handle_;
  }


  bool is_open () const noexcept
  {
    return handle_ != invalid_socket;
  }


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


  void open (const protocol_t &protocol)
  {
    __bits::error_guard guard("basic_socket::open");
    open(protocol, guard);
  }


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


  void assign (const protocol_t &protocol, const native_handle_t &handle)
  {
    __bits::error_guard guard("basic_socket::assign");
    assign(protocol, handle, guard);
  }


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


  void close ()
  {
    __bits::error_guard guard("basic_socket::close");
    close(guard);
  }


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


  template <typename GettableSocketOption>
  void get_option (const GettableSocketOption &option) const
  {
    __bits::error_guard guard("basic_socket::get_option");
    get_option(option, guard);
  }


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


  template <typename SettableSocketOption>
  void set_option (const SettableSocketOption &option)
  {
    __bits::error_guard guard("basic_socket::set_option");
    set_option(option, guard);
  }


  void non_blocking (bool mode, std::error_code &error) noexcept
  {
    __bits::non_blocking(handle_, mode, error);
  }


  void non_blocking (bool mode)
  {
    __bits::error_guard guard("basic_socket::non_blocking");
    non_blocking(mode, guard);
  }


  bool non_blocking (std::error_code &error) const noexcept
  {
    return __bits::non_blocking(handle_, error);
  }


  bool non_blocking () const
  {
    __bits::error_guard guard("basic_socket::non_blocking");
    return non_blocking(guard);
  }


  size_t available (std::error_code &error) const noexcept
  {
    return __bits::available(handle_, error);
  }


  size_t available () const
  {
    __bits::error_guard guard("basic_socket::available");
    return available(guard);
  }


  void bind (const endpoint_t &endpoint, std::error_code &error) noexcept
  {
    __bits::bind(handle_, endpoint.data(), endpoint.size(), error);
  }


  void bind (const endpoint_t &endpoint)
  {
    __bits::error_guard guard("basic_socket::bind");
    bind(endpoint, guard);
  }


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


  endpoint_t local_endpoint () const
  {
    __bits::error_guard guard("basic_socket::local_endpoint");
    return local_endpoint(guard);
  }


#if 0
  remote_endpoint;
  connect;
  shutdown;
  wait;
#endif


protected:

  basic_socket_t () = default;


  basic_socket_t (basic_socket_t &&that) noexcept
    : handle_(that.handle_)
  {
    that.handle_ = invalid_socket;
  }


  ~basic_socket_t () noexcept
  {
    if (is_open())
    {
      std::error_code ignored;
      close(ignored);
    }
  }


  basic_socket_t (const protocol_t &protocol)
  {
    open(protocol);
  }


  basic_socket_t (const endpoint_t &endpoint)
    : basic_socket_t(endpoint.protocol())
  {
    bind(endpoint);
  }


  basic_socket_t (const protocol_t &protocol, const native_handle_t &handle)
  {
    assign(protocol, handle);
  }


#if 0
  template <typename OtherProtocol>
  basic_socket_t (basic_socket_t<OtherProtocol> &&)
  {
  }


  template <typename OtherProtocol>
  basic_socket_t &operator= (basic_socket_t<OtherProtocol> &&)
  {
    return *this;
  }
#endif


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
