#pragma once

/**
 * \file sal/crypto/channel.hpp
 * Secure encrypted channel using TLS/DTLS.
 */


#include <sal/config.hpp>
#include <sal/crypto/__bits/channel.hpp>
#include <sal/crypto/channel_options.hpp>
#include <sal/crypto/error.hpp>
#include <sal/memory.hpp>


__sal_begin


namespace crypto {


/**
 * Secure encrypted two-way communication channel using TLS or DTLS.
 * This class is transport independent protocol parser/generator: feed packets
 * with handshake/encrypt/decrypt and it invokes callback with ready messages.
 *
 * To set up new channel, both parties must call handshake() repeatedly and
 * send generated messages back and forth until session and it's settings are
 * agreed (is_connected() will return true on both sides). To initiate
 * negotiations, handshake() must be called first on client side (with no
 * input data).
 *
 * Once session is established, new invocations of handshake() will generate
 * error (currently re-negotiation is not supported). In this state,
 * application can call encrypt() to generate secret text from plain text and
 * decrypt() for reverse operation.
 *
 * \see https://en.wikipedia.org/wiki/Transport_Layer_Security
 */
class channel_t
{
public:

  /**
   * Callbacks for handshake/encrypted/decrypted buffers. Channel allocates
   * new buffers with alloc() and returns those buffers to transport layer
   * with ready().
   */
  struct buffer_manager_t
  {
    virtual ~buffer_manager_t () noexcept = default;

    /**
     * Callback to allocate new buffer. On return, set \a buffer to pointer of
     * new memory area with \a buffer_size where output message will be
     * stored. If generated message does not fit into \a buffer, this callback
     * is called multiple times for new chunks until whole message is stored.
     * For each buffer, ready() will be called separately.
     *
     * Return value uintptr_t can be used by application layer to tie each
     * returned buffer with application-specific data. It is not used by
     * channel except storing and returning back to application in call to
     * ready().
     */
    virtual uintptr_t alloc (uint8_t **buffer, size_t *buffer_size) noexcept = 0;

    /**
     * Callback for stored message (or chunks if message is split between
     * multiple buffers). \a Buffer contains pointer returned by alloc() and
     * \a buffer_size has number of actually stored bytes. It's value might be
     * zero if channel did not generate any output, in which case \a buffer
     * can be simply released.
     *
     * For meaning of \a user_data, see alloc()
     */
    virtual void ready (uintptr_t user_data, uint8_t *buffer, size_t buffer_size) noexcept = 0;
  };


  /**
   * Return true if all the handshakes are successfully finished and channel
   * is ready of encrypt/decrypt operations.
   */
  bool is_connected () const noexcept
  {
    return impl_->handshake_status == std::errc::already_connected;
  }


  /**
   * Proceed with session negotiations. \a data must contain message(s) sent
   * by remote party. \a buffer_manager is used to allocate buffers for output
   * messages (or it's chunks). Once is_connected() returns true, current side
   * has finished with handshakes.
   *
   * \a data may contain more than current handshake stage needed. In this
   * case, method returns number of bytes actually used. It is application
   * responsibility to remove those bytes from \a data and feed remaining
   * content again to handshake() (possibly with additional bytes from remote
   * party).
   *
   * There is also special case on client side: \a data may contain both
   * handshake and application data. In this case, application should remove
   * used \a data and if is_connected() is true, call decrypt() with remaining
   * \a data.
   *
   * In case of negotiations failure, set \a error to describe error
   * situation. During handshake phase, all errors are final i.e. negotiation
   * will be stopped.
   */
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


  /**
   * \see handshake()
   * Instead of setting \a error, throw std::system_error
   */
  template <typename Data>
  size_t handshake (const Data &data, buffer_manager_t &buffer_manager)
  {
    return handshake(data, buffer_manager,
      throw_on_error("channel::handshake")
    );
  }


  /**
   * Encrypt \a data, setting \a error on failure. Generated secret text is
   * stored in buffer(s) allocated by \a buffer_manager.
   *
   * This method can succeed only when is_connected() returns true.
   */
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


  /**
   * \see encrypt()
   * Instead of setting \a error, throw std::system_error
   */
  template <typename Data>
  void encrypt (const Data &data, buffer_manager_t &buffer_manager)
  {
    encrypt(data, buffer_manager, throw_on_error("channel::encrypt"));
  }


  /**
   * Decrypt secret text in \a data returning plain text in buffer(s)
   * allocated by \a buffer_manager. On failure, set \a error. Returns number
   * of bytes actually used from \a data. It is application's responsibility
   * to remove used \a data and invoke this method again with remaining of
   * \a data (with possible additional data from remote peer).
   *
   * This method can succeed only when is_connected() returns true.
   */
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


  /**
   * \see decrypt()
   * Instead of setting \a error, throw std::system_error
   */
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
 * Factory class for creating new secure channel_t.
 *
 * It constructs one time internal setup (certificate chain, etc) that will be
 * reused to create multiple similar channel_t instances (a la acceptor on
 * server side and so on).
 *
 * \a Datagram specifies whether to use datagrams (DTLS) or streaming (TLS)
 * approach to handle exchanged messages.
 *
 * \a Server specifies whether it's server side (i.e. acceptor) or client side
 * (i.e. connector).
 */
template <bool Datagram, bool Server>
class channel_factory_t
{
public:

  /**
   * Construct new channel factory with number of \a options (none, one or
   * more).
   *
   * \see channel_factory_option_t
   */
  template <typename... Option>
  channel_factory_t (const channel_factory_option_t<Option> &...options)
    : impl_(std::make_shared<__bits::channel_factory_t>(Datagram, Server))
  {
    (set_option(static_cast<const Option &>(options)), ...);
    impl_->ctor(throw_on_error("channel_factory"));
  }


  /**
   * Construct new channel with number of channel \a options (none, one or
   * more).
   *
   * \see channel_option_t
   */
  template <typename... Option>
  channel_t make_channel (const channel_option_t<Option> &...options)
  {
    auto channel = std::make_unique<__bits::channel_t>(impl_);
    (set_option(*channel, static_cast<const Option &>(options)), ...);
    channel->ctor(throw_on_error("channel_factory::make_channel"));
    return channel;
  }


private:

  __bits::channel_factory_ptr impl_{};

  void set_option (const with_chain_t &option) noexcept
  {
    impl_->chain = option.value;
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
  void set_option (const chain_check_t<Check> &option) noexcept
  {
    impl_->chain_check = option.value;
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


/**
 * TLS client side channel factory.
 */
using stream_client_channel_factory_t = channel_factory_t<false, false>;

/**
 * TLS server side channel factory.
 */
using stream_server_channel_factory_t = channel_factory_t<false, true>;

/**
 * DTLS client side channel factory.
 */
using datagram_client_channel_factory_t = channel_factory_t<true, false>;

/**
 * DTLS server side channel factory.
 */
using datagram_server_channel_factory_t = channel_factory_t<true, true>;


/**
 * Construct and return new TLS client side channel factory using \a options.
 */
template <typename... Option>
inline stream_client_channel_factory_t stream_client_channel_factory (
  const channel_factory_option_t<Option> &...options)
{
  return {options...};
}


/**
 * Construct and return new TLS server side channel factory using \a options.
 */
template <typename... Option>
inline stream_server_channel_factory_t stream_server_channel_factory (
  const channel_factory_option_t<Option> &...options)
{
  return {options...};
}


/**
 * Construct and return new DTLS client side channel factory using \a options.
 */
template <typename... Option>
inline datagram_client_channel_factory_t datagram_client_channel_factory (
  const channel_factory_option_t<Option> &...options)
{
  return {options...};
}


/**
 * Construct and return new DTLS server side channel factory using \a options.
 */
template <typename... Option>
inline datagram_server_channel_factory_t datagram_server_channel_factory (
  const channel_factory_option_t<Option> &...options)
{
  return {options...};
}


} // namespace crypto


__sal_end
