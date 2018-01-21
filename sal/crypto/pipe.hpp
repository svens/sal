#pragma once

/**
 * \file sal/crypto/pipe.hpp
 * Secure encrypted channel.
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/pipe.hpp>
#include <sal/crypto/pipe_options.hpp>
#include <sal/crypto/error.hpp>


__sal_begin


namespace crypto {


class pipe_t
{
public:

  bool is_connected () const noexcept
  {
    return impl_ && impl_->state == &__bits::pipe_t::connected;
  }


  template <typename It>
  pipe_t &receive_buffer (It first, It last) noexcept
  {
    impl_->recv_first = to_ptr(first);
    impl_->recv_last = to_end_ptr(first, last);
    return *this;
  }


  template <typename Data>
  pipe_t &receive_buffer (const Data &data) noexcept
  {
    using std::cbegin;
    using std::cend;
    return receive_buffer(cbegin(data), cend(data));
  }


  pipe_t &receive_buffer (
    const std::pair<const uint8_t *, const uint8_t *> &buffer) noexcept
  {
    impl_->recv_first = buffer.first;
    impl_->recv_last = buffer.second;
    return *this;
  }


  template <typename It>
  pipe_t &send_buffer (It first, It last) noexcept
  {
    impl_->send_first = impl_->send_ptr = to_ptr(first);
    impl_->send_last = to_end_ptr(first, last);
    return *this;
  }


  template <typename Data>
  pipe_t &send_buffer (Data &data) noexcept
  {
    using std::begin;
    using std::end;
    return send_buffer(begin(data), end(data));
  }


  pipe_t &send_buffer (const std::pair<uint8_t *, uint8_t *> &buffer)
    noexcept
  {
    impl_->send_first = impl_->send_ptr = buffer.first;
    impl_->send_last = buffer.second;
    return *this;
  }


  void process (std::error_code &error) noexcept
  {
    impl_->process(error);
  }


  void process ()
  {
    process(throw_on_error("pipe::process"));
  }


  std::pair<const uint8_t *, const uint8_t *> receive_buffer () const noexcept
  {
    return {impl_->recv_first, impl_->recv_last};
  }


  std::pair<const uint8_t *, const uint8_t *> send_buffer () const noexcept
  {
    return {impl_->send_first, impl_->send_ptr};
  }


private:

  __bits::pipe_ptr impl_;

  pipe_t (__bits::pipe_ptr &&pipe) noexcept
    : impl_(std::move(pipe))
  {}

  template <bool Server>
  friend class basic_pipe_factory_t;
};


template <bool Server>
class basic_pipe_factory_t
{
public:

  template <typename... Option>
  basic_pipe_factory_t (const pipe_factory_option_t<Option> &...option)
  {
    (set_option(static_cast<const Option &>(option)), ...);
    impl_->ctor(throw_on_error("basic_pipe_factory"));
  }


  template <typename... Option>
  pipe_t make_stream_pipe (const pipe_option_t<Option> &...option)
  {
    auto pipe = impl_->make_pipe(true);
    (set_option(*pipe, static_cast<const Option &>(option)), ...);
    pipe->ctor(throw_on_error("basic_pipe_factory::make_stream_pipe"));
    return pipe;
  }


  template <typename... Option>
  pipe_t make_datagram_pipe (const pipe_option_t<Option> &...option)
  {
    auto pipe = impl_->make_pipe(false);
    (set_option(*pipe, static_cast<const Option &>(option)), ...);
    pipe->ctor(throw_on_error("basic_pipe_factory::make_datagram_pipe"));
    return pipe;
  }


private:

  __bits::pipe_factory_ptr impl_ = std::make_shared<__bits::pipe_factory_t>(Server);


  void set_option (const mutual_auth_t &) noexcept
  {
    impl_->mutual_auth = true;
  }


  void set_option (const with_certificate_t &option) noexcept
  {
    impl_->certificate = option.certificate.native_handle();
  }


  template <typename Check>
  void set_option (const manual_certificate_check_t<Check> &option) noexcept
  {
    impl_->certificate_check = option.check;
  }


  void set_option (__bits::pipe_t &pipe, const peer_name_t &option)
  {
    pipe.peer_name = option.peer_name;
  }
};


using client_pipe_factory_t = basic_pipe_factory_t<false>;
using server_pipe_factory_t = basic_pipe_factory_t<true>;


template <typename... Option>
server_pipe_factory_t server_pipe_factory (
  const pipe_factory_option_t<Option> &...options)
{
  return {options...};
}


template <typename... Option>
client_pipe_factory_t client_pipe_factory (
  const pipe_factory_option_t<Option> &...options)
{
  return {options...};
}


} // namespace crypto


__sal_end
