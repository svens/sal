#pragma once

/**
 * \file sal/net/async/service.hpp
 * Asynchronous networking service
 */


#include <sal/config.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/net/async/io.hpp>


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


private:

  __bits::service_ptr impl_ = std::make_shared<__bits::service_t>();
};


} // namespace net::async


__sal_end
