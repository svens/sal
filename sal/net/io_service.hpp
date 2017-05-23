#pragma once

/**
 * \file sal/net/io_service.hpp
 *
 * Asynchronous I/O completion service.
 */


#include <sal/config.hpp>
#include <sal/net/__bits/async_socket.hpp>
#include <sal/net/basic_socket.hpp>
#include <sal/net/basic_socket_acceptor.hpp>
#include <sal/net/error.hpp>
#include <sal/net/io_context.hpp>


__sal_begin


namespace net {


/**
 * Asynchronous networking I/O completion service.
 *
 * This class holds platform-dependent completion handler (IOCP/epoll/kqueue)
 * but it is not meant to be used directly for polling completions. Instead,
 * per-thread io_context_t does actual completion waiting and resources
 * management.
 */
class io_service_t
{
public:

  /**
   * Create new I/O completion service.
   */
  io_service_t ()
    : impl_(throw_on_error("io_service"))
  {}


  /**
   * Create new I/O completion thread context.
   *
   * \a max_events_per_wait configures how many events are batched when
   * waiting for completions. If this number is too little, then underlying
   * syscall must be called more often (with kernel- and user-mode switch
   * overhead). On too big value and/or if each completion handling takes too
   * long, it might take too long time to get to handling specific completion.
   */
  io_context_t make_context (size_t max_events_per_wait = 16)
  {
    if (max_events_per_wait < 1)
    {
      max_events_per_wait = 1;
    }
    else if (max_events_per_wait > impl_.max_events_per_wait)
    {
      max_events_per_wait = impl_.max_events_per_wait;
    }
    return io_context_t(impl_, max_events_per_wait);
  }


  /**
   * Associate \a socket with this io_service_t. Calling asynchronous \a
   * socket methods without associating it first will generate error.
   * On failure, set \a error
   */
  template <typename Protocol>
  void associate (basic_socket_t<Protocol> &socket, std::error_code &error)
    noexcept
  {
    impl_.associate(socket.socket_, error);
  }


  /**
   * Associate \a socket with this io_service_t. Calling asynchronous \a
   * socket methods without associating it first will generate error.
   * On failure, throw \c std::system_error
   */
  template <typename Protocol>
  void associate (basic_socket_t<Protocol> &socket)
  {
    associate(socket, throw_on_error("io_service::associate"));
  }


  /// \copydoc associate(basic_socket_t<Protocol> &, std::error_code&)
  template <typename Protocol>
  void associate (basic_socket_acceptor_t<Protocol> &socket,
    std::error_code &error) noexcept
  {
    impl_.associate(socket.socket_, error);
  }


  /// \copydoc associate(basic_socket_t<Protocol> &)
  template <typename Protocol>
  void associate (basic_socket_acceptor_t<Protocol> &socket)
  {
    associate(socket, throw_on_error("io_service::associate"));
  }


private:

  __bits::io_service_t impl_;
};


} // namespace net


__sal_end
