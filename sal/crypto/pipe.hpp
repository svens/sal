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
    return impl_->handshake_result == std::errc::already_connected;
  }


  template <typename In, typename Out>
  std::pair<size_t, size_t> handshake (const In &in, Out &&out,
    std::error_code &error) noexcept
  {
    using std::begin;
    using std::end;
    using std::cbegin;
    using std::cend;
    return impl_->handshake(
      to_ptr(cbegin(in)), to_end_ptr(cbegin(in), cend(in)),
      to_ptr(begin(out)), to_end_ptr(begin(out), end(out)),
      error
    );
  }


  template <typename In, typename Out>
  std::pair<size_t, size_t> handshake (const In &in, Out &&out)
  {
    return handshake(in, out, throw_on_error("pipe::handshake"));
  }


  template <typename In, typename Out>
  std::pair<size_t, size_t> encrypt (const In &in, Out &&out,
    std::error_code &error) noexcept
  {
    using std::begin;
    using std::end;
    using std::cbegin;
    using std::cend;
    return impl_->encrypt(
      to_ptr(cbegin(in)), to_end_ptr(cbegin(in), cend(in)),
      to_ptr(begin(out)), to_end_ptr(begin(out), end(out)),
      error
    );
  }


  template <typename In, typename Out>
  std::pair<size_t, size_t> encrypt (const In &in, Out &&out)
  {
    return encrypt(in, out, throw_on_error("pipe::encrypt"));
  }


  template <typename In, typename Out>
  std::pair<size_t, size_t> decrypt (const In &in, Out &&out,
    std::error_code &error) noexcept
  {
    using std::begin;
    using std::end;
    using std::cbegin;
    using std::cend;
    return impl_->decrypt(
      to_ptr(cbegin(in)), to_end_ptr(cbegin(in), cend(in)),
      to_ptr(begin(out)), to_end_ptr(begin(out), end(out)),
      error
    );
  }


  template <typename In, typename Out>
  std::pair<size_t, size_t> decrypt (const In &in, Out &&out)
  {
    return decrypt(in, out, throw_on_error("pipe::decrypt"));
  }


private:

  __bits::pipe_ptr impl_;

  pipe_t (__bits::pipe_ptr &&pipe) noexcept
    : impl_(std::move(pipe))
  {}

  template <bool Server, bool Datagram>
  friend class basic_pipe_factory_t;
};


template <bool Server, bool Datagram>
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
  pipe_t make_pipe (const pipe_option_t<Option> &...option)
  {
    auto pipe = impl_->make_pipe();
    (set_option(*pipe, static_cast<const Option &>(option)), ...);
    pipe->ctor(throw_on_error("basic_pipe_factory::make_pipe"));
    return pipe;
  }


private:

  __bits::pipe_factory_ptr impl_ = std::make_shared<__bits::pipe_factory_t>(
    Server,
    Datagram
  );


  void set_option (const mutual_auth_t &) noexcept
  {
    impl_->mutual_auth = true;
  }


  void set_option (const with_certificate_t &option) noexcept
  {
    impl_->certificate = option.certificate.native_handle();
  }


  void set_option (const with_private_key_t &option) noexcept
  {
#if __sal_os_linux
    // used only on Linux, other platforms keep private key in secured memory
    impl_->private_key = option.private_key->native_handle().ref;
#else
    (void)option;
#endif
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


using stream_client_pipe_factory_t = basic_pipe_factory_t<false, false>;
using stream_server_pipe_factory_t = basic_pipe_factory_t<true, false>;
using datagram_client_pipe_factory_t = basic_pipe_factory_t<false, true>;
using datagram_server_pipe_factory_t = basic_pipe_factory_t<true, true>;


template <typename... Option>
stream_server_pipe_factory_t stream_server_pipe_factory (
  const pipe_factory_option_t<Option> &...options)
{
  return {options...};
}


template <typename... Option>
stream_client_pipe_factory_t stream_client_pipe_factory (
  const pipe_factory_option_t<Option> &...options)
{
  return {options...};
}


template <typename... Option>
datagram_server_pipe_factory_t datagram_server_pipe_factory (
  const pipe_factory_option_t<Option> &...options)
{
  return {options...};
}


template <typename... Option>
datagram_client_pipe_factory_t datagram_client_pipe_factory (
  const pipe_factory_option_t<Option> &...options)
{
  return {options...};
}


} // namespace crypto


__sal_end
