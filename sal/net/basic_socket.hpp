#pragma once

/**
 * \file sal/net/basic_socket.hpp
 * Datagram and stream sockets' base class
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/error.hpp>
#include <sal/net/socket_base.hpp>

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
    return handle_ != no_handle;
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
      throw_system_error(error, "open");
    }
  }


  void assign (const protocol_t &,
    const native_handle_t &handle,
    std::error_code &error) noexcept
  {
    if (handle == no_handle)
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
      throw_system_error(error, "assign");
    }
  }


  void close (std::error_code &error) noexcept
  {
    if (is_open())
    {
      socket_base_t::close(handle_, error);
      handle_ = no_handle;
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
      throw_system_error(error, "close");
    }
  }


protected:

  basic_socket_t () = default;


  basic_socket_t (basic_socket_t &&that) noexcept
    : handle_(that.handle_)
  {
    that.handle_ = no_handle;
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
  basic_socket_t (const endpoint_t &)
  {
  }
#endif


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
    that.handle_ = no_handle;
    return *this;
  }


  basic_socket_t (const basic_socket_t &) = delete;
  basic_socket_t &operator= (const basic_socket_t &) = delete;


private:

  native_handle_t handle_ = no_handle;
};


} // namespace net


__sal_end
