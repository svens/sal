#pragma once

/**
 * \file sal/net/async/service.hpp
 * Asynchronous networking service
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/net/async/io.hpp>
#include <chrono>


__sal_begin


namespace net::async {


class service_t
{
public:

  size_t io_pool_size () const noexcept
  {
    return impl_->io_pool_size;
  }


  io_t make_io ()
  {
    return {impl_->make_io(nullptr, type_v<std::nullptr_t>)};
  }


  template <typename Context>
  io_t make_io (Context *context)
  {
    return {impl_->make_io(context, type_v<Context>)};
  }


  io_t try_get () noexcept
  {
    return {impl_->dequeue()};
  }


  template <typename Rep, typename Period>
  io_t poll (const std::chrono::duration<Rep, Period> &timeout,
    std::error_code &error) noexcept
  {
    return {impl_->poll(timeout, error)};
  }


  template <typename Rep, typename Period>
  io_t poll (const std::chrono::duration<Rep, Period> &timeout)
  {
    return poll(timeout, throw_on_error("service::poll"));
  }


  io_t poll (std::error_code &error) noexcept
  {
    return poll((std::chrono::milliseconds::max)(), error);
  }


  io_t poll ()
  {
    return poll((std::chrono::milliseconds::max)());
  }


  io_t try_poll (std::error_code &error) noexcept
  {
    return poll(std::chrono::milliseconds(0), error);
  }


  io_t try_poll ()
  {
    return poll(std::chrono::milliseconds(0));
  }


private:

  __bits::service_ptr impl_ = std::make_shared<__bits::service_t>();

  template <typename Protocol> friend class net::basic_socket_t;
  template <typename Protocol> friend class net::basic_socket_acceptor_t;
};


} // namespace net::async


__sal_end
