#pragma once

/**
 * \file sal/crypto/pipe.hpp
 * Secure encrypted channel.
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/pipe.hpp>
#include <sal/crypto/certificate.hpp>
#include <sal/crypto/error.hpp>
#include <variant>


#include <iostream>


__sal_begin


namespace crypto {


/**
 */
class pipe_t
{
public:

  pipe_t () noexcept = default;


  void encrypt ()
  {
  }


  void decrypt ()
  {
  }


private:

  __bits::pipe_ptr impl_{};

  pipe_t (__bits::pipe_ptr &&pipe) noexcept
    : impl_(std::move(pipe))
  {}

  template <typename Send>
  friend class pipe_handshake_t;
};


/**
 */
template <typename Send>
class pipe_handshake_t
  : private __bits::pipe_event_handler_t
{
public:

  template <typename It>
  void process (It first, It last, std::error_code &error) noexcept
  {
    if (impl_)
    {
      impl_->process(to_ptr(first), to_end_ptr(first, last), error);
    }
    else
    {
      error = std::make_error_code(std::errc::already_connected);
    }
  }


  template <typename It>
  void process (It first, It last)
  {
    process(first, last, throw_on_error("pipe_handshake::process"));
  }


  template <typename Data>
  void process (const Data &data, std::error_code &error) noexcept
  {
    using std::cbegin;
    using std::cend;
    process(cbegin(data), cend(data), error);
  }


  template <typename Data>
  void process (const Data &data)
  {
    using std::cbegin;
    using std::cend;
    process(cbegin(data), cend(data));
  }


  pipe_t pipe (std::error_code &error) noexcept
  {
    if (impl_ && impl_->max_message_size_ > 0)
    {
      return std::move(impl_);
    }
    error = std::make_error_code(std::errc::not_connected);
    return {};
  }


  pipe_t pipe ()
  {
    return pipe(throw_on_error("pipe_handshake::pipe"));
  }


private:

  __bits::pipe_ptr impl_;
  Send send_;

  template <typename Data>
  pipe_handshake_t (__bits::pipe_ptr &&pipe, Data &send_buffer, Send send)
    : impl_(std::move(pipe))
    , send_(send)
  {
    using std::begin;
    using std::end;
    auto first = to_ptr(begin(send_buffer));
    auto last = to_end_ptr(begin(send_buffer), end(send_buffer));
    impl_->start_handshake(this, first, last,
      throw_on_error("pipe_handshake")
    );
  }

  void pipe_on_send (const uint8_t *first, const uint8_t *last,
    std::error_code &error) noexcept final override
  {
    send_(first, last, error);
  }

  friend class pipe_factory_t;
};


/**
 */
class pipe_factory_t
{
public:

  template <typename Option>
  struct option_t {};


  struct inbound_t: public option_t<inbound_t> {};
  struct outbound_t: public option_t<outbound_t> {};
  struct mutual_auth_t: public option_t<mutual_auth_t> {};


  struct with_certificate_t
    : option_t<with_certificate_t>
  {
    certificate_t certificate;

    with_certificate_t (const certificate_t &certificate) noexcept
      : certificate(certificate)
    {}
  };


  template <typename Validator>
  struct manual_certificate_check_t
    : option_t<manual_certificate_check_t<Validator>>
  {
    Validator validator;

    manual_certificate_check_t (Validator validator) noexcept
      : validator(validator)
    {}
  };


  template <typename... Option>
  pipe_factory_t (const option_t<Option> &...option)
  {
    (set_option(static_cast<const Option &>(option)), ...);
    impl_->ctor(throw_on_error("pipe_factory"));
  }


  template <typename Data, typename Send>
  pipe_handshake_t<Send> make_stream_pipe (Data &send_buffer, Send send)
  {
    return
    {
      std::make_unique<__bits::pipe_t>(impl_, true),
      send_buffer,
      send
    };
  }


  template <typename Data, typename Send>
  pipe_handshake_t<Send> make_datagram_pipe (Data &send_buffer, Send send)
  {
    return
    {
      std::make_unique<__bits::pipe_t>(impl_, false),
      send_buffer,
      send
    };
  }


private:

  __bits::pipe_factory_ptr impl_ = std::make_shared<__bits::pipe_factory_t>();


  void set_option (const with_certificate_t &option) noexcept
  {
    impl_->certificate_ = option.certificate.native_handle();
  }


  void set_option (const mutual_auth_t &) noexcept
  {
    impl_->mutual_auth_ = true;
  }


  void set_option (const inbound_t &) noexcept
  {
    impl_->inbound_ = true;
  }


  void set_option (const outbound_t &) noexcept
  {
    impl_->inbound_ = false;
  }


  template <typename Validator>
  void set_option (const manual_certificate_check_t<Validator> &option) noexcept
  {
    impl_->manual_certificate_check_ = option.validator;
  }
};


/**
 */
inline constexpr const pipe_factory_t::inbound_t inbound{};
inline constexpr const pipe_factory_t::outbound_t outbound{};
inline constexpr const pipe_factory_t::mutual_auth_t mutual_auth{};


/**
 */
inline pipe_factory_t::with_certificate_t with_certificate (
  const certificate_t &certificate) noexcept
{
  return {certificate};
}


/**
 */
template <typename Validator>
inline pipe_factory_t::manual_certificate_check_t<Validator>
  manual_certificate_check (Validator validator) noexcept
{
  return {validator};
}


inline const auto no_certificate_check = manual_certificate_check(
  [](const certificate_t &) noexcept
  {
    return true;
  }
);


/**
 */
template <typename... Option>
inline pipe_factory_t pipe_factory (const pipe_factory_t::option_t<Option> &...options)
{
  return {options...};
}


} // namespace crypto


__sal_end
