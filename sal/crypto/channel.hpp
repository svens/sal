#pragma once

/**
 * \file sal/crypto/channel.hpp
 * Secure encrypted channel.
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/channel.hpp>
#include <sal/crypto/channel_options.hpp>
#include <sal/crypto/error.hpp>
#include <sal/memory.hpp>


__sal_begin


namespace crypto {


/**
 */
class channel_t
{
public:

  struct buffer_manager_t
  {
    virtual ~buffer_manager_t () noexcept
    { }

    virtual uintptr_t alloc (uint8_t **buffer, size_t *buffer_size) noexcept = 0;
    virtual void ready (uintptr_t user_data, uint8_t *buffer, size_t buffer_size) noexcept = 0;
  };


  bool is_connected () const noexcept
  {
    return impl_->handshake_status == std::errc::already_connected;
  }


  template <typename Data>
  size_t handshake (const Data &data,
    buffer_manager_t &buffer_manager,
    std::error_code &error) noexcept
  {
    if (impl_->handshake_status == std::errc::not_connected)
    {
      using std::cbegin;
      using std::cend;
      return handshake(
        to_ptr(cbegin(data)),
        range_size(cbegin(data), cend(data)),
        buffer_manager,
        error
      );
    }
    error = impl_->handshake_status;
    return {};
  }


  template <typename Data>
  size_t handshake (const Data &data, buffer_manager_t &buffer_manager)
  {
    return handshake(data, buffer_manager,
      throw_on_error("channel::handshake")
    );
  }


  template <typename Data>
  void encrypt (const Data &data,
    buffer_manager_t &buffer_manager,
    std::error_code &error) noexcept
  {
    if (impl_->handshake_status == std::errc::already_connected)
    {
      using std::cbegin;
      using std::cend;
      encrypt(
        to_ptr(cbegin(data)),
        range_size(cbegin(data), cend(data)),
        buffer_manager,
        error
      );
    }
    else
    {
      error = impl_->handshake_status;
    }
  }


  template <typename Data>
  void encrypt (const Data &data, buffer_manager_t &buffer_manager)
  {
    encrypt(data, buffer_manager, throw_on_error("channel::encrypt"));
  }


  template <typename Data>
  size_t decrypt (const Data &data,
    buffer_manager_t &buffer_manager,
    std::error_code &error) noexcept
  {
    if (impl_->handshake_status == std::errc::already_connected)
    {
      using std::cbegin;
      using std::cend;
      return decrypt(
        to_ptr(cbegin(data)),
        range_size(cbegin(data), cend(data)),
        buffer_manager,
        error
      );
    }
    error = impl_->handshake_status;
    return {};
  }


  template <typename Data>
  size_t decrypt (const Data &data, buffer_manager_t &buffer_manager)
  {
    return decrypt(data, buffer_manager, throw_on_error("channel::decrypt"));
  }


private:

  __bits::channel_ptr impl_;

  channel_t (__bits::channel_ptr &&impl) noexcept
    : impl_(std::move(impl))
  { }

  size_t handshake (const uint8_t *data, size_t size,
    buffer_manager_t &buffer_manager,
    std::error_code &error
  ) noexcept;

  void encrypt (const uint8_t *data, size_t size,
    buffer_manager_t &buffer_manager,
    std::error_code &error
  ) noexcept;

  size_t decrypt (const uint8_t *data, size_t size,
    buffer_manager_t &buffer_manager,
    std::error_code &error
  ) noexcept;

  template <bool Datagram, bool Server>
  friend class channel_factory_t;
};


/**
 */
template <bool Datagram, bool Server>
class channel_factory_t
{
public:

  template <typename... Option>
  channel_factory_t (const channel_factory_option_t<Option> &...option)
    : impl_(std::make_shared<__bits::channel_factory_t>(Datagram, Server))
  {
    (set_option(static_cast<const Option &>(option)), ...);
    impl_->ctor(throw_on_error("channel_factory"));
  }


  template <typename... Option>
  channel_t make_channel (const channel_option_t<Option> &...option)
  {
    auto channel = std::make_unique<__bits::channel_t>(impl_);
    (set_option(*channel, static_cast<const Option &>(option)), ...);
    channel->ctor(throw_on_error("channel_factory::make_channel"));
    return channel;
  }


private:

  __bits::channel_factory_ptr impl_{};

  void set_option (const with_certificate_t &option) noexcept
  {
    impl_->certificate = option.value.native_handle();
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
    impl_->certificate_check = option.value;
  }

  void set_option (__bits::channel_t &channel, const mutual_auth_t &option)
    noexcept
  {
    channel.mutual_auth = option.value;
  }

  void set_option (__bits::channel_t &channel, const peer_name_t &option)
  {
    channel.peer_name = option.value;
  }
};


using stream_client_channel_factory_t = channel_factory_t<false, false>;
using stream_server_channel_factory_t = channel_factory_t<false, true>;
using datagram_client_channel_factory_t = channel_factory_t<true, false>;
using datagram_server_channel_factory_t = channel_factory_t<true, true>;


/**
 */
template <typename... Option>
inline stream_client_channel_factory_t stream_client_channel_factory (
  const channel_factory_option_t<Option> &...option)
{
  return {option...};
}


/**
 */
template <typename... Option>
inline stream_server_channel_factory_t stream_server_channel_factory (
  const channel_factory_option_t<Option> &...option)
{
  return {option...};
}


/**
 */
template <typename... Option>
inline datagram_client_channel_factory_t datagram_client_channel_factory (
  const channel_factory_option_t<Option> &...option)
{
  return {option...};
}


/**
 */
template <typename... Option>
inline datagram_server_channel_factory_t datagram_server_channel_factory (
  const channel_factory_option_t<Option> &...option)
{
  return {option...};
}


} // namespace crypto


__sal_end
