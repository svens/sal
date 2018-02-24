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


#include <iostream>


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
    return impl_->status == std::errc::already_connected;
  }


  template <typename Data>
  size_t handshake (const Data &data,
    buffer_manager_t &buffer_manager,
    std::error_code &error) noexcept
  {
    if (impl_->status == std::errc::not_connected)
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
    error = impl_->status;
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
    if (impl_->status == std::errc::already_connected)
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
      error = impl_->status;
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
    if (impl_->status == std::errc::already_connected)
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
    error = impl_->status;
    return {};
  }


  template <typename Data>
  size_t decrypt (const Data &data, buffer_manager_t &buffer_manager)
  {
    return decrypt(data, buffer_manager, throw_on_error("channel::decrypt"));
  }


private:

  __bits::channel_ptr impl_;

  channel_t (__bits::channel_context_ptr context, bool server)
    : impl_(std::make_unique<__bits::channel_t>(context, server))
  { }

  void ctor (std::error_code &error) noexcept;

  void set_option (const peer_name_t &option)
  {
    impl_->peer_name = option.value;
  }

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

  friend class channel_context_t;
};


/**
 */
class channel_context_t
{
public:

  template <typename... Option>
  channel_context_t (const channel_context_option_t<Option> &...option)
    : impl_(std::make_shared<__bits::channel_context_t>())
  {
    (set_option(static_cast<const Option &>(option)), ...);
    ctor(throw_on_error("channel_context"));
  }


  template <typename... Option>
  channel_t accept (const channel_option_t<Option> &...option)
  {
    channel_t channel{impl_, true};
    (channel.set_option(static_cast<const Option &>(option)), ...);
    channel.ctor(throw_on_error("channel_context::accept"));
    return channel;
  }


  template <typename... Option>
  channel_t connect (const channel_option_t<Option> &...option)
  {
    channel_t channel{impl_, false};
    (channel.set_option(static_cast<const Option &>(option)), ...);
    channel.ctor(throw_on_error("channel_context::connect"));
    return channel;
  }


private:

  __bits::channel_context_ptr impl_{};

  void ctor (std::error_code &error) noexcept;

  void set_option (const datagram_oriented_t &option) noexcept
  {
    impl_->datagram = option.value;
  }

  void set_option (const mutual_auth_t &option) noexcept
  {
    impl_->mutual_auth = option.value;
  }

  void set_option (const with_certificate_t &option) noexcept
  {
    impl_->certificate = option.value;
  }

  void set_option (const with_private_key_t &option) noexcept
  {
#if __sal_os_linux
    // used only on Linux, other platforms keep private key in secured memory
    private_key_ = option.private_key->native_handle().ref;
#else
    (void)option;
#endif
  }

  template <typename Check>
  void set_option (const manual_certificate_check_t<Check> &option) noexcept
  {
    impl_->certificate_check = option.value;
  }
};


/**
 */
template <typename... Option>
inline channel_context_t channel_context (
  const channel_context_option_t<Option> &...option)
{
  return {option...};
}


} // namespace crypto


__sal_end
