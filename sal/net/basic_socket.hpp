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
      handle_ = socket_base_t::open(protocol.family(),
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
    std::error_code error;
    open(protocol, error);
    if (error)
    {
      throw_system_error(error, "basic_socket::open");
    }
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
    std::error_code error;
    assign(protocol, handle, error);
    if (error)
    {
      throw_system_error(error, "basic_socket::assign");
    }
  }


  void close (std::error_code &error) noexcept
  {
    if (is_open())
    {
      socket_base_t::close(handle_, error);
      handle_ = invalid_socket;
    }
    else
    {
      error = make_error_code(std::errc::bad_file_descriptor);
    }
  }


  void close ()
  {
    std::error_code error;
    close(error);
    if (error)
    {
      throw_system_error(error, "basic_socket::close");
    }
  }


  template <typename GettableSocketOption>
  void get_option (const GettableSocketOption &option, std::error_code &error)
    const noexcept
  {
    typename GettableSocketOption::native_t data;
    socklen_t size = sizeof(data);
    socket_base_t::get_opt(handle_,
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
    std::error_code error;
    get_option(option, error);
    if (error)
    {
      throw_system_error(error, "basic_socket::get_option");
    }
  }


  template <typename SettableSocketOption>
  void set_option (const SettableSocketOption &option,
    std::error_code &error) noexcept
  {
    typename SettableSocketOption::native_t data;
    option.store(data);
    socket_base_t::set_opt(handle_,
      option.level, option.name,
      &data, sizeof(data),
      error
    );
  }


  template <typename SettableSocketOption>
  void set_option (const SettableSocketOption &option)
  {
    std::error_code error;
    set_option(option, error);
    if (error)
    {
      throw_system_error(error, "basic_socket::set_option");
    }
  }


#if 0
  io_control;
  non_blocking;
  native_non_blocking;
  at_mark;
  available;
  bind;
  shutdown;
  local_endpoint;
  remote_endpoint;
  connect;
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


  basic_socket_t (const protocol_t &protocol, const native_handle_t &handle)
  {
    assign(protocol, handle);
  }


#if 0
  basic_socket_t (const endpoint_t &endpoint)
    : basic_socket_t(endpoint.protocol())
  {
    bind(endpoint);
  }


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
