#pragma once

/**
 * \file sal/net/async/basic_socket.hpp
 */


#include <sal/config.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/net/async/service.hpp>
#include <sal/net/basic_socket.hpp>
#include <sal/assert.hpp>


__sal_begin


namespace net::async {


template <typename Protocol>
class basic_socket_t
  : public net::basic_socket_t<Protocol>
{
  using base_t = net::basic_socket_t<Protocol>;

public:

  using handle_t = typename base_t::handle_t;
  using protocol_t = typename base_t::protocol_t;
  using endpoint_t = typename base_t::endpoint_t;


  void associate (service_t &service,
    size_t max_outstanding_receives,
    size_t max_outstanding_sends,
    std::error_code &error
  ) noexcept;


  void associate (service_t &service,
    size_t max_outstanding_receives,
    size_t max_outstanding_sends)
  {
    associate(service,
      max_outstanding_receives,
      max_outstanding_sends,
      throw_on_error("basic_socket::associate")
    );
  }


  template <typename Context>
  void context (Context *context)
  {
    auto &impl = *sal_check_ptr(impl_);
    impl.context = context;
    impl.context_type = type_v<Context>;
  }


  template <typename Context>
  Context *context () const
  {
    auto &impl = *sal_check_ptr(impl_);
    if (impl.context_type == type_v<Context>)
    {
      return static_cast<Context *>(impl.context);
    }
    return nullptr;
  }


protected:

  __bits::async_socket_ptr impl_{};


  basic_socket_t () noexcept = default;
  basic_socket_t (basic_socket_t &&) noexcept = default;
  basic_socket_t &operator= (basic_socket_t &&) noexcept = default;
  ~basic_socket_t () noexcept = default;


  basic_socket_t (handle_t handle)
    : base_t(handle)
  {}


  basic_socket_t (const Protocol &protocol)
    : basic_socket_t(
        __bits::async_socket_t::open(
          protocol.family(),
          protocol.type(),
          protocol.protocol()
        )
      )
  {}


  basic_socket_t (const typename Protocol::endpoint_t &endpoint)
    : basic_socket_t(endpoint.protocol())
  {
    base_t::bind(endpoint);
  }


  static __bits::io_t &acquire (io_t &&io) noexcept
  {
    return *io.impl_.release();
  }


  template <typename Result>
  static Result *result_storage (io_t &io) noexcept
  {
    static_assert(std::is_trivially_destructible_v<Result>);
    static_assert(sizeof(Result) <= sizeof(io.impl_->result_data));
    io.impl_->result_type = type_v<Result>;
    return reinterpret_cast<Result *>(io.impl_->result_data);
  }


  template <typename Result>
  static const Result *result_of (const io_t &io, std::error_code &error)
    noexcept
  {
    if (io.impl_->result_type == type_v<Result>)
    {
      error = io.impl_->status;
      return reinterpret_cast<const Result *>(io.impl_->result_data);
    }
    return nullptr;
  }
};


template <typename Protocol>
void basic_socket_t<Protocol>::associate (service_t &service,
  size_t max_outstanding_receives,
  size_t max_outstanding_sends,
  std::error_code &error) noexcept
{
  if (!base_t::is_open())
  {
    error = make_error_code(std::errc::bad_file_descriptor);
    return;
  }
  else if (impl_)
  {
    error = make_error_code(socket_errc::already_associated);
    return;
  }

  impl_.reset(new(std::nothrow)
    __bits::async_socket_t(
      base_t::native_handle(),
      service.impl_,
      max_outstanding_receives,
      max_outstanding_sends,
      error
    )
  );
  if (!impl_)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  else if (error)
  {
    impl_.reset();
  }
}


} // namespace net::async


__sal_end
