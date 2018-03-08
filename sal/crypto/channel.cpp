#include <sal/crypto/channel.hpp>

#if __sal_os_linux //{{{1
  #include <mutex>
  #include <openssl/err.h>
  #include <openssl/opensslv.h>
#elif __sal_os_macos //{{{1
  #include <Security/SecIdentity.h>
#elif __sal_os_windows //{{{1
  #include <schannel.h>
  #include <security.h>
  #pragma comment(lib, "secur32")
#endif //}}}1


#define WITH_LOGGING 0
#if WITH_LOGGING
  #include <iostream>
  #define LOG(x) x
#else
  #define LOG(x)
#endif


__sal_begin


namespace crypto {


#if __sal_os_linux //{{{1


namespace {


constexpr bool openssl_pre_1_1 () noexcept
{
  return OPENSSL_VERSION_NUMBER < 0x10100000;
}


inline void init_openssl () noexcept
{
  if constexpr (openssl_pre_1_1())
  {
    static std::once_flag init_flag;
    std::call_once(init_flag,
      []()
      {
        ::SSL_library_init();
        ::SSL_load_error_strings();
      }
    );
  }
}


inline const SSL_METHOD *channel_type (bool datagram, bool server) noexcept
{
  static const SSL_METHOD * const method[] =
  {
#if OPENSSL_VERSION_NUMBER < 0x10100000
    ::TLSv1_client_method(),
    ::TLSv1_server_method(),
    ::DTLSv1_client_method(),
    ::DTLSv1_server_method(),
#else
    ::TLS_client_method(),
    ::TLS_server_method(),
    ::DTLS_client_method(),
    ::DTLS_server_method(),
#endif
  };
  return method[datagram * 2 + server];
}


bool set_certificate (__bits::channel_factory_t &factory,
  std::error_code &error) noexcept
{
  if (factory.certificate.ref)
  {
    auto result = ::SSL_CTX_use_certificate(
      factory.handle.ref,
      factory.certificate.ref
    );
    if (result != 1)
    {
      error.assign(::ERR_get_error(), category());
      return false;
    }

    result = ::SSL_CTX_use_PrivateKey(
      factory.handle.ref,
      factory.private_key
    );
    if (result != 1)
    {
      error.assign(::ERR_get_error(), category());
      return false;
    }

    result = ::SSL_CTX_check_private_key(factory.handle.ref);
    if (result != 1)
    {
      error.assign(::ERR_get_error(), category());
      return false;
    }
  }
  return true;
}


bool set_read_ahead (__bits::channel_factory_t &factory, std::error_code &)
  noexcept
{
  ::SSL_CTX_set_read_ahead(factory.handle.ref, 0);
  return true;
}


#if WITH_LOGGING
void info_callback (const SSL *ssl, int where, int ret)
{
  if (ret != 0)
  {
    #define I_(w) \
      do { \
        if (where & SSL_CB_ ## w) \
        { \
          std::cout << "    | " #w " " << ::SSL_state_string_long(ssl) << '\n'; \
        } \
      } while (false)
    I_(LOOP);
    I_(READ);
    I_(WRITE);
    I_(ALERT);
    I_(HANDSHAKE_START);
    I_(HANDSHAKE_DONE);
    #undef I_
  }
  else
  {
    std::cout << "    * error " << ret << '\n';
  }
}
#endif


bool to_ssl (__bits::channel_t &channel,
  const uint8_t **data,
  size_t *size,
  std::error_code &error) noexcept
{
  if (*size > 0)
  {
    *size = ::BIO_write(channel.in, *data, *size);
    if (*size > 0)
    {
      LOG(std::cout << "    | in " << *size << '\n');
      return true;
    }
    else if (::BIO_should_retry(channel.in))
    {
      LOG(std::cout << "    | in: retry\n");
      return true;
    }
    else
    {
      LOG(std::cout << "    | in: no retry\n");
      error.assign(::ERR_get_error(), category());
      return false;
    }
  }
  return true;
}


bool from_ssl (__bits::channel_t &channel,
  channel_t::buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  while (::BIO_ctrl_pending(channel.out) > 0)
  {
    size_t chunk_size{};
    uint8_t *chunk_ptr{};
    auto user_data = buffer_manager.alloc(&chunk_ptr, &chunk_size);
    if (chunk_ptr && chunk_size)
    {
      auto size = ::BIO_read(channel.out, chunk_ptr, chunk_size);
      if (size > 0)
      {
        LOG(std::cout << "    | out " << size << '\n');
        buffer_manager.ready(user_data, chunk_ptr, size);
        continue;
      }
      else if (::BIO_should_retry(channel.out))
      {
        LOG(std::cout << "    | out: retry\n");
        return true;
      }
      else
      {
        LOG(std::cout << "    | out: no retry\n");
        error.assign(::ERR_get_error(), category());
        return false;
      }
    }
    else
    {
      error = std::make_error_code(std::errc::no_buffer_space);
      return false;
    }
  }
  return true;
}


} // namespace


void __bits::channel_factory_t::ctor (std::error_code &error) noexcept
{
  init_openssl();

  handle = ::SSL_CTX_new(channel_type(datagram, server));
  if (handle)
  {
    if (set_certificate(*this, error)
      && set_read_ahead(*this, error))
    {
      error.clear();
    }
  }
  else
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
}


__bits::channel_factory_t::~channel_factory_t () noexcept
{ }


void __bits::channel_t::ctor (std::error_code &error) noexcept
{
  handle = ::SSL_new(factory->handle.ref);
  if (handle)
  {
    if (factory->server)
    {
      ::SSL_set_accept_state(handle.ref);
    }
    else
    {
      ::SSL_set_connect_state(handle.ref);
    }
    LOG(::SSL_set_info_callback(handle.ref, info_callback));

    in = ::BIO_new(::BIO_s_mem());
    out = ::BIO_new(::BIO_s_mem());

    if (in && out)
    {
      ::BIO_set_mem_eof_return(in, -1);
      ::BIO_set_mem_eof_return(out, -1);
      ::SSL_set_bio(handle.ref, in, out);
      error.clear();
      return;
    }

    ::BIO_free_all(in);
    ::BIO_free_all(out);
  }
  error = std::make_error_code(std::errc::not_enough_memory);
}


__bits::channel_t::~channel_t () noexcept
{ }


size_t channel_t::handshake (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  auto &channel = *impl_;

  LOG(std::cout << (channel.factory->server ? "server" : "client")
    << "> handshake: " << size << '\n'
  );

  auto used_size = size;
  if (!to_ssl(channel, &data, &used_size, error))
  {
    return used_size;
  }

  auto status = ::SSL_do_handshake(channel.handle.ref);
  switch (::SSL_get_error(channel.handle.ref, status))
  {
    case SSL_ERROR_NONE:
      LOG(std::cout << "    | connected\n");
      channel.connected();
      if (!::BIO_ctrl_pending(channel.out))
      {
        error.clear();
        break;
      }
      [[fallthrough]];

    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      if (from_ssl(channel, buffer_manager, error))
      {
        error.clear();
      }
      break;

    case SSL_ERROR_ZERO_RETURN:
      LOG(std::cout << "    | aborted\n");
      channel.abort();
      error.clear();
      break;

    case SSL_ERROR_SSL:
    case SSL_ERROR_SYSCALL:
      channel.abort();
      error.assign(::ERR_get_error(), category());
      LOG(std::cout << "    | error: " << error.message() << "\n");
      break;

    default:
      LOG(std::cout << "    | unhandled " << status << '\n');
      break;
  }

  return used_size;
}


void channel_t::encrypt (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  auto &channel = *impl_;

  LOG(std::cout << (channel.factory->server ? "server" : "client")
    << "> encrypt: " << size << '\n'
  );

  auto status = ::SSL_write(channel.handle.ref, data, size);
  switch (::SSL_get_error(channel.handle.ref, status))
  {
    case SSL_ERROR_NONE:
      if (from_ssl(channel, buffer_manager, error))
      {
        error.clear();
      }
      break;

    case SSL_ERROR_WANT_WRITE:
      LOG(std::cout << "    | want_write\n");
      error.clear();
      break;

    case SSL_ERROR_WANT_READ:
      LOG(std::cout << "    | want_read\n");
      error.clear();
      break;

    case SSL_ERROR_ZERO_RETURN:
      LOG(std::cout << "    | clean close\n");
      break;

    case SSL_ERROR_SSL:
      LOG(std::cout << "    | error_ssl\n");
      break;

    case SSL_ERROR_SYSCALL:
      LOG(std::cout << "    | error_syscall\n");
      break;

    default:
      LOG(std::cout << "    | unhandled " << status << '\n');
      break;
  }
}


size_t channel_t::decrypt (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  auto &channel = *impl_;

  LOG(std::cout << (channel.factory->server ? "server" : "client")
    << "> decrypt: " << size << '\n'
  );

  auto used_size = size;
  if (!to_ssl(channel, &data, &used_size, error))
  {
    return {};
  }

  for (;;)
  {
    size_t chunk_size{};
    uint8_t *chunk_ptr{};
    auto user_data = buffer_manager.alloc(&chunk_ptr, &chunk_size);
    if (!chunk_ptr || !chunk_size)
    {
      error = std::make_error_code(std::errc::no_buffer_space);
      break;
    }

    auto status = ::SSL_read(channel.handle.ref, chunk_ptr, chunk_size);
    switch (::SSL_get_error(channel.handle.ref, status))
    {
      case SSL_ERROR_NONE:
        LOG(std::cout << "    | ready " << status << '\n');
        buffer_manager.ready(user_data, chunk_ptr, status);
        LOG(std::cout << "    | pending"
          << " BIO.in=" << ::BIO_ctrl_pending(channel.in)
          << "; BIO.win=" << ::BIO_ctrl_wpending(channel.in)
          << "; BIO.out=" << ::BIO_ctrl_pending(channel.out)
          << "; BIO.wout=" << ::BIO_ctrl_wpending(channel.out)
          << "; SSL=" << SSL_pending(channel.handle.ref)
          << '\n'
        );
        if (auto not_used = ::BIO_ctrl_pending(channel.in))
        {
          BIO_reset(channel.in);
          return used_size - not_used;
        }
        continue;

      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        LOG(std::cout << "    | want_io\n");
        if (chunk_ptr)
        {
          buffer_manager.ready(user_data, chunk_ptr, 0U);
        }
        return used_size;

      case SSL_ERROR_ZERO_RETURN:
        LOG(std::cout << "    | clean close\n");
        return {};

      case SSL_ERROR_SSL:
        LOG(std::cout << "    | error_ssl\n");
        return {};

      case SSL_ERROR_SYSCALL:
        LOG(std::cout << "    | error_syscall\n");
        return {};

      default:
        LOG(std::cout << "    | unhandled " << status << '\n');
        return {};
    }
  }

  return {};
}


#elif __sal_os_macos //{{{1


namespace {


struct crypto_call_t
{
  const uint8_t *in_first;
  const uint8_t *in_last;
  const uint8_t *in_ptr;
  channel_t::buffer_manager_t &buffer_manager;


  crypto_call_t (const uint8_t *first,
      const uint8_t *last,
      channel_t::buffer_manager_t &buffer_manager) noexcept
    : in_first(first)
    , in_last(last)
    , in_ptr(first)
    , buffer_manager(buffer_manager)
  { }


  ::OSStatus handshake (__bits::channel_t &channel) noexcept
  {
    channel.syscall = this;
    return ::SSLHandshake(channel.handle.ref);
  }


  ::OSStatus encrypt (__bits::channel_t &channel, size_t *processed) noexcept
  {
    channel.syscall = this;
    return ::SSLWrite(channel.handle.ref,
      in_ptr,
      in_last - in_ptr,
      processed
    );
  }


  ::OSStatus decrypt (__bits::channel_t &channel, size_t *processed) noexcept;
  void drain (__bits::channel_t &channel, size_t *processed) noexcept;

  crypto_call_t (const crypto_call_t &) = delete;
  crypto_call_t &operator= (const crypto_call_t &) = delete;
};


inline __bits::channel_t &to_channel (::SSLConnectionRef connection) noexcept
{
  return *const_cast<__bits::channel_t *>(
    static_cast<const __bits::channel_t *>(connection)
  );
}


::OSStatus crypto_call_t::decrypt (__bits::channel_t &channel,
  size_t *processed) noexcept
{
  channel.syscall = this;

  uint8_t *chunk_ptr{};
  size_t chunk_size{};
  auto user_data = buffer_manager.alloc(&chunk_ptr, &chunk_size);
  if (chunk_ptr && chunk_size)
  {
    size_t read{};
    auto status = ::SSLRead(channel.handle.ref, chunk_ptr, chunk_size, &read);
    buffer_manager.ready(user_data, chunk_ptr, read);
    if (status == ::errSecSuccess)
    {
      *processed += read;
    }
    return status;
  }

  return ::errSSLBufferOverflow;
}


void crypto_call_t::drain (__bits::channel_t &channel, size_t *processed)
  noexcept
{
  size_t buffered_size{};
  ::SSLGetBufferedReadSize(channel.handle.ref, &buffered_size);

  while (buffered_size)
  {
    uint8_t *chunk_ptr{};
    size_t chunk_size{};
    auto user_data = buffer_manager.alloc(&chunk_ptr, &chunk_size);
    if (chunk_ptr && chunk_size)
    {
      size_t read{};
      auto status = ::SSLRead(channel.handle.ref, chunk_ptr, chunk_size, &read);
      buffer_manager.ready(user_data, chunk_ptr, read);
      if (status == ::errSecSuccess)
      {
        *processed += read;
        buffered_size -= read;
        continue;
      }
    }
    return;
  }
}


::OSStatus ssl_read (::SSLConnectionRef connection,
  void *data, size_t *size) noexcept
{
  LOG(std::cout << "    | read " << *size);

  auto &channel = to_channel(connection);
  auto &call = *static_cast<crypto_call_t *>(channel.syscall);

  if (call.in_ptr < call.in_last)
  {
    ::OSStatus status = ::errSecSuccess;
    size_t have = call.in_last - call.in_ptr;
    if (have < *size)
    {
      *size = have;
      status = ::errSSLWouldBlock;
      LOG(std::cout << ", less: " << have << '\n');
    }
    else
    {
      LOG(std::cout << ", all\n");
    }
    std::uninitialized_copy_n(call.in_ptr, *size, static_cast<uint8_t *>(data));
    call.in_ptr += *size;
    return status;
  }

  LOG(std::cout << ", empty\n");
  *size = 0;
  return ::errSSLWouldBlock;
}


::OSStatus ssl_write (::SSLConnectionRef connection,
  const void *data, size_t *size) noexcept
{
  LOG(std::cout << "    | write " << *size);

  auto &channel = to_channel(connection);
  auto &call = *static_cast<crypto_call_t *>(channel.syscall);
  auto data_ptr = static_cast<const uint8_t *>(data);
  auto data_size = *size;
  *size = 0;

  while (data_size)
  {
    uint8_t *chunk_ptr{};
    size_t chunk_size{};
    auto user_data = call.buffer_manager.alloc(&chunk_ptr, &chunk_size);
    if (chunk_ptr && chunk_size)
    {
      if (chunk_size > data_size)
      {
        chunk_size = data_size;
      }
      std::uninitialized_copy_n(data_ptr, chunk_size, chunk_ptr);
      call.buffer_manager.ready(user_data, chunk_ptr, chunk_size);
      data_ptr += chunk_size;
      data_size -= chunk_size;
      *size += chunk_size;
    }
    else
    {
      LOG(std::cout << ", no buf\n");
      return ::errSSLBufferOverflow;
    }
  }

  LOG(std::cout << ", all\n");
  return ::errSecSuccess;
}


bool set_io (__bits::channel_t &channel, std::error_code &error) noexcept
{
  auto status = ::SSLSetIOFuncs(channel.handle.ref, &ssl_read, &ssl_write);
  if (status == ::errSecSuccess)
  {
    return true;
  }
  error.assign(status, category());
  return false;
}


bool set_connection (__bits::channel_t &channel, std::error_code &error)
  noexcept
{
  auto status = ::SSLSetConnection(channel.handle.ref, &channel);
  if (status == ::errSecSuccess)
  {
    return true;
  }
  error.assign(status, category());
  return false;
}


bool set_peer_name (__bits::channel_t &channel, std::error_code &error)
  noexcept
{
  if (!channel.peer_name.empty())
  {
    auto status = ::SSLSetPeerDomainName(channel.handle.ref,
      channel.peer_name.data(),
      channel.peer_name.size()
    );
    if (status != ::errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool set_mutual_auth (__bits::channel_t &channel, std::error_code &error)
  noexcept
{
  if (channel.mutual_auth)
  {
    auto status = ::SSLSetClientSideAuthenticate(channel.handle.ref,
      ::kAlwaysAuthenticate
    );
    if (status != ::errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool set_certificate (__bits::channel_t &channel, std::error_code &error)
  noexcept
{
  if (channel.factory->certificate)
  {
    unique_ref<::SecIdentityRef> identity;
    auto status = ::SecIdentityCreateWithCertificate(nullptr,
      channel.factory->certificate.ref,
      &identity.ref
    );
    if (status != ::errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }

    unique_ref<::CFArrayRef> certificates = ::CFArrayCreate(nullptr,
      (::CFTypeRef *)&identity.ref, 1,
      &::kCFTypeArrayCallBacks
    );
    if (!certificates.ref)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
      return false;
    }

    status = ::SSLSetCertificate(channel.handle.ref, certificates.ref);
    if (status != ::errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool set_certificate_check (__bits::channel_t &channel, std::error_code &error)
  noexcept
{
  if (channel.factory->certificate_check)
  {
    auto break_on_auth = channel.factory->server
      ? ::kSSLSessionOptionBreakOnClientAuth
      : ::kSSLSessionOptionBreakOnServerAuth
    ;
    auto status = ::SSLSetSessionOption(channel.handle.ref, break_on_auth, true);
    if (status != ::errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool is_trusted_peer (__bits::channel_t &channel, std::error_code &error)
  noexcept
{
  unique_ref<::SecTrustRef> trust{};
  auto status = ::SSLCopyPeerTrust(channel.handle.ref, &trust.ref);
  if (status != ::errSecSuccess)
  {
    error.assign(status, category());
    return false;
  }

  auto peer_certificate = crypto::certificate_t::from_native_handle(
    ::SecTrustGetCertificateAtIndex(trust.ref, 0)
  );
  ::CFRetain(peer_certificate.native_handle().ref);

  return channel.factory->certificate_check(peer_certificate);
}


} // namespace


void __bits::channel_factory_t::ctor (std::error_code &error) noexcept
{
  error.clear();
}


__bits::channel_factory_t::~channel_factory_t () noexcept
{ }


void __bits::channel_t::ctor (std::error_code &error) noexcept
{
  handle.ref = ::SSLCreateContext(nullptr,
    factory->server ? ::kSSLServerSide : ::kSSLClientSide,
    factory->datagram ? ::kSSLDatagramType : ::kSSLStreamType
  );
  if (!handle)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }

  if (set_io(*this, error)
    && set_connection(*this, error)
    && set_peer_name(*this, error)
    && set_mutual_auth(*this, error)
    && set_certificate(*this, error)
    && set_certificate_check(*this, error))
  {
    error.clear();
  }
}


__bits::channel_t::~channel_t () noexcept
{ }


size_t channel_t::handshake (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  auto &channel = *impl_;

  LOG(std::cout << (channel.factory->server ? "server" : "client")
    << "> handshake: " << size << '\n'
  );

  crypto_call_t call(data, data + size, buffer_manager);
  for (;;)
  {
    switch (auto status = call.handshake(channel))
    {
      case ::errSecSuccess:
        LOG(std::cout << "    | connected (" << (call.in_ptr - call.in_first) << ")\n");
        channel.connected();
        error.clear();
        break;

      case ::errSSLWouldBlock:
        LOG(std::cout << "    | blocked\n");
        error.clear();
        break;

      case ::errSSLBufferOverflow:
        LOG(std::cout << "    | overflow\n");
        error = std::make_error_code(std::errc::no_buffer_space);
        break;

      case ::errSSLPeerAuthCompleted:
        LOG(std::cout << "    | certificate_check\n");
        if (!is_trusted_peer(channel, error))
        {
          channel.abort();
          break;
        }
        continue;

      default:
        LOG(std::cout << "    | error\n");
        channel.abort();
        error.assign(status, category());
        break;
    }

    return call.in_ptr - call.in_first;
  }
}


void channel_t::encrypt (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  LOG(std::cout << (impl_->factory->server ? "server" : "client")
    << "> encrypt: " << size << '\n'
  );

  size_t processed{};
  crypto_call_t call(data, data + size, buffer_manager);
  switch (auto status = call.encrypt(*impl_, &processed))
  {
    case ::errSecSuccess:
      LOG(std::cout << "    | ready " << processed << "\n");
      error.clear();
      return;

    case ::errSSLBufferOverflow:
      LOG(std::cout << "    | overflow\n");
      error = std::make_error_code(std::errc::no_buffer_space);
      return;

    default:
      LOG(std::cout << "    | error\n");
      error.assign(status, category());
      return;
  }
}


size_t channel_t::decrypt (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  LOG(std::cout << (impl_->factory->server ? "server" : "client")
    << "> decrypt: " << size << '\n'
  );

  size_t processed{};
  crypto_call_t call(data, data + size, buffer_manager);
  switch (auto status = call.decrypt(*impl_, &processed))
  {
    case ::errSecSuccess:
      call.drain(*impl_, &processed);
      LOG(std::cout << "    | ready " << processed
        << ", used " << (call.in_ptr - call.in_first)
        << "\n"
      );
      error.clear();
      break;

    case ::errSSLWouldBlock:
      LOG(std::cout << "    | blocked " << processed << "\n");
      error.clear();
      break;

    case ::errSSLBufferOverflow:
      LOG(std::cout << "    | overflow\n");
      error = std::make_error_code(std::errc::no_buffer_space);
      break;

    default:
      LOG(std::cout << "    | error\n");
      error.assign(status, category());
      break;
  }

  return call.in_ptr - call.in_first;
}


#elif __sal_os_windows //{{{1


namespace {


inline auto handle_result (SECURITY_STATUS status, const char *func)
{
#if WITH_LOGGING //{{{2
  std::cout << "    > " << func << ": ";
  #define S_(s) case SEC_##s: std::cout << #s; break
  switch (status)
  {
    S_(E_OK);
    S_(E_ALGORITHM_MISMATCH);
    S_(E_APPLICATION_PROTOCOL_MISMATCH);
    S_(E_BUFFER_TOO_SMALL);
    S_(E_CERT_EXPIRED);
    S_(E_CERT_UNKNOWN);
    S_(E_DECRYPT_FAILURE);
    S_(E_ENCRYPT_FAILURE);
    S_(E_ILLEGAL_MESSAGE);
    S_(E_INCOMPLETE_CREDENTIALS);
    S_(E_INCOMPLETE_MESSAGE);
    S_(E_INSUFFICIENT_MEMORY);
    S_(E_INTERNAL_ERROR);
    S_(E_INVALID_HANDLE);
    S_(E_INVALID_PARAMETER);
    S_(E_INVALID_TOKEN);
    S_(E_LOGON_DENIED);
    S_(E_MESSAGE_ALTERED);
    S_(E_MUTUAL_AUTH_FAILED);
    S_(E_NO_AUTHENTICATING_AUTHORITY);
    S_(E_NO_CREDENTIALS);
    S_(E_OUT_OF_SEQUENCE);
    S_(E_TARGET_UNKNOWN);
    S_(E_UNFINISHED_CONTEXT_DELETED);
    S_(E_UNSUPPORTED_FUNCTION);
    S_(E_UNTRUSTED_ROOT);
    S_(E_WRONG_PRINCIPAL);
    S_(I_COMPLETE_AND_CONTINUE);
    S_(I_COMPLETE_NEEDED);
    S_(I_CONTEXT_EXPIRED);
    S_(I_CONTINUE_NEEDED);
    S_(I_CONTINUE_NEEDED_MESSAGE_OK);
    S_(I_INCOMPLETE_CREDENTIALS);
    S_(I_MESSAGE_FRAGMENT);
    S_(I_RENEGOTIATE);
    default: std::cout << "Error<" << std::hex << status << ">";
  }
  #undef S_
  std::cout << '\n';
#else
  (void)func;
#endif //}}}2
  return status;
}
#define call(func, ...) handle_result(func(__VA_ARGS__), #func)


struct buffer_t
  : public ::SecBuffer
{
  buffer_t (int type, uint8_t *p, size_t size) noexcept
  {
    BufferType = type;
    pvBuffer = p;
    cbBuffer = static_cast<ULONG>(size);
  }

  struct list_t
    : public ::SecBufferDesc
  {
    template <ULONG N>
    list_t (buffer_t (&bufs)[N]) noexcept
    {
      ulVersion = SECBUFFER_VERSION;
      pBuffers = bufs;
      cBuffers = N;
    }

    buffer_t &operator[] (size_t index) noexcept
    {
      return static_cast<buffer_t &>(pBuffers[index]);
    }
  };
};


template <int Type>
struct basic_buffer_t
  : public buffer_t
{
  basic_buffer_t () noexcept
    : buffer_t(Type, nullptr, 0U)
  { }

  basic_buffer_t (uint8_t *p, size_t size) noexcept
    : buffer_t(Type, p, size)
  {}

  basic_buffer_t (const uint8_t *p, size_t size) noexcept
    : basic_buffer_t(const_cast<uint8_t *>(p), size)
  {}
};

using header_t = basic_buffer_t<SECBUFFER_STREAM_HEADER>;
using trailer_t = basic_buffer_t<SECBUFFER_STREAM_TRAILER>;
using data_t = basic_buffer_t<SECBUFFER_DATA>;
using empty_t = basic_buffer_t<SECBUFFER_EMPTY>;
using alert_t = basic_buffer_t<SECBUFFER_ALERT>;
using extra_t = basic_buffer_t<SECBUFFER_EXTRA>;
using token_t = basic_buffer_t<SECBUFFER_TOKEN>;


void print_bufs (const char prefix[], buffer_t::list_t bufs) noexcept
{
#if WITH_LOGGING //{{{2
  std::cout << "    | " << prefix << ':';
  auto n = 0U;
  for (auto *it = bufs.pBuffers;  it != bufs.pBuffers + bufs.cBuffers;  ++it, ++n)
  {
    std::cout << ' ' << n << '=';
    #define S_(s) case SECBUFFER_##s: std::cout << #s; break
    switch (it->BufferType)
    {
      S_(STREAM_HEADER);
      S_(STREAM_TRAILER);
      S_(DATA);
      S_(EMPTY);
      S_(ALERT);
      S_(TOKEN);
      S_(EXTRA);
      S_(MISSING);
      S_(STREAM);
      default: std::cout << "XXX_" << it->BufferType; break;
    }
    #undef S_
    std::cout << '<' << it->cbBuffer << '>';
  }
  std::cout << '\n';
#else
  (void)prefix;
  (void)bufs;
#endif //}}}2
}


void print_flags (ULONG flags)
{
#if WITH_LOGGING //{{{2
  std::cout << "  Flags:";
  #define F_(f) if ((flags & ISC_REQ_##f) == ISC_REQ_##f) std::cout << " " #f;
  F_(ALLOCATE_MEMORY);
  F_(CONFIDENTIALITY);
  F_(CONNECTION);
  F_(DATAGRAM);
  F_(DELEGATE);
  F_(EXTENDED_ERROR);
  F_(IDENTIFY);
  F_(INTEGRITY);
  F_(MANUAL_CRED_VALIDATION);
  F_(MUTUAL_AUTH);
  F_(NO_INTEGRITY);
  F_(PROMPT_FOR_CREDS);
  F_(REPLAY_DETECT);
  F_(SEQUENCE_DETECT);
  F_(STREAM);
  F_(USE_DCE_STYLE);
  F_(USE_SESSION_KEY);
  F_(USE_SUPPLIED_CREDS);
  #undef F_
  std::cout << '\n';
#else
  (void)flags;
#endif //}}}2
}


} // namespace


void __bits::channel_factory_t::ctor (std::error_code &error) noexcept
{
  ::SCHANNEL_CRED auth_data{};
  auth_data.dwVersion = SCHANNEL_CRED_VERSION;

  auth_data.dwFlags = SCH_CRED_NO_DEFAULT_CREDS;
  if (certificate_check)
  {
    auth_data.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;
  }

  if (!certificate.is_null())
  {
    auth_data.paCred = &certificate.ref;
    auth_data.cCreds = 1;
  }

  auto status = ::AcquireCredentialsHandle(
    nullptr,
    UNISP_NAME,
    (server ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND),
    nullptr,
    &auth_data,
    nullptr,
    nullptr,
    &credentials,
    nullptr
  );

  if (status == SEC_E_OK)
  {
    error.clear();
  }
  else
  {
    error.assign(status, category());
  }
}


__bits::channel_factory_t::~channel_factory_t () noexcept
{
  (void)::FreeCredentialsHandle(&credentials);
}


void __bits::channel_t::ctor (std::error_code &error) noexcept
{
  if (factory->server)
  {
    if (factory->datagram)
    {
      context_request |= ASC_REQ_DATAGRAM;
    }
    else
    {
      context_request |= ASC_REQ_STREAM;
    }
    if (mutual_auth)
    {
      context_request |= ASC_REQ_MUTUAL_AUTH;
    }
  }
  else
  {
    if (factory->datagram)
    {
      context_request |= ISC_REQ_DATAGRAM;
    }
    else
    {
      context_request |= ISC_REQ_STREAM;
    }
    if (mutual_auth)
    {
      context_request |= ISC_REQ_MUTUAL_AUTH;
    }
  }
  error.clear();
}


__bits::channel_t::~channel_t () noexcept
{
  if (handle_p)
  {
    (void)::DeleteSecurityContext(handle_p);
  }
}


namespace {


bool buffer_while_incomplete_message (__bits::channel_t &channel,
  const uint8_t **data,
  size_t *size,
  std::error_code &error) noexcept
{
  // add new data to buffer
  /*
  LOG(std::cout << "    | buffer: " << channel.in.size()
    << " + " << *size
  );
  */

  try
  {
    channel.in.insert(channel.in.end(), *data, *data + *size);
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return true;
  }

  *data = channel.in.data();
  *size = channel.in.size();
  //LOG(std::cout << " -> " << channel.in.size());

  if (channel.in.size() < channel.complete_message_size)
  {
    // not enough yet
    //LOG(std::cout << ", need more\n");
    return true;
  }

  if (channel.factory->datagram
    && channel.factory->server
    && channel.in.size() < 13)
  {
    // XXX special case for DTLS server side:
    // feeding less than 13B will make SChannel handshake fail with illegal
    // message (which we don't want to handle in handshake() method
    // So, just keep buffering until we have enough
    //LOG(std::cout << ", special buffering\n");
    return true;
  }

  // enough, set input to buffer and reset expected message size
  //LOG(std::cout << ", ready\n");
  channel.complete_message_size = 0;
  return false;
}


bool flush (__bits::channel_t &channel,
  channel_t::buffer_manager_t &buffer_manager,
  const uint8_t *ptr,
  size_t size,
  std::error_code &error) noexcept
{
  (void)channel;

  LOG(std::cout << "    | flush " << size);
  while (size)
  {
    uint8_t *chunk_ptr{};
    size_t chunk_size{};
    auto user_data = buffer_manager.alloc(&chunk_ptr, &chunk_size);
    if (chunk_ptr && chunk_size)
    {
      if (chunk_size > size)
      {
        chunk_size = size;
      }
      std::uninitialized_copy_n(ptr, chunk_size,
        sal::__bits::make_output_iterator(chunk_ptr, chunk_ptr + chunk_size)
      );
      buffer_manager.ready(user_data, chunk_ptr, chunk_size);
      size -= chunk_size;
      ptr += chunk_size;
    }
    else
    {
      LOG(std::cout << ", remaining " << size << '\n');
      error = std::make_error_code(std::errc::no_buffer_space);
      return false;
    }
  }
  LOG(std::cout << ", done\n");
  return true;
}


bool handle_token (__bits::channel_t &channel,
  buffer_t &token,
  channel_t::buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  if (token.BufferType == SECBUFFER_TOKEN && token.pvBuffer && token.cbBuffer)
  {
    auto size = token.cbBuffer;
    auto ptr = static_cast<const uint8_t *>(token.pvBuffer);
    return flush(channel, buffer_manager, ptr, size, error);
  }
  return true;
}


bool handle_data (__bits::channel_t &channel,
  buffer_t &data,
  channel_t::buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  if (data.BufferType == SECBUFFER_DATA && data.pvBuffer && data.cbBuffer)
  {
    auto size = data.cbBuffer;
    auto ptr = static_cast<const uint8_t *>(data.pvBuffer);
    return flush(channel, buffer_manager, ptr, size, error);
  }
  return true;
}


size_t handle_extra (__bits::channel_t &channel, buffer_t &extra) noexcept
{
  channel.in.clear();
  if (extra.BufferType == SECBUFFER_EXTRA)
  {
    return extra.cbBuffer;
  }
  return 0;
}


bool handle_missing (__bits::channel_t &channel,
  buffer_t &missing,
  const uint8_t *data,
  size_t data_size,
  std::error_code &error) noexcept
{
  if (missing.BufferType == SECBUFFER_MISSING && missing.cbBuffer)
  {
    channel.complete_message_size = missing.cbBuffer + channel.in.size();
    if (channel.complete_message_size > channel.max_message_size)
    {
      error = std::make_error_code(std::errc::no_buffer_space);
      return false;
    }
    try
    {
      channel.in.reserve(channel.complete_message_size);
    }
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
      return false;
    }
  }
  if (channel.in.empty())
  {
    buffer_while_incomplete_message(channel, &data, &data_size, error);
    if (error)
    {
      return false;
    }
  }
  return true;
}


bool is_trusted_peer (__bits::channel_t &channel, std::error_code &error)
  noexcept
{
  if (channel.factory->certificate_check)
  {
    __bits::certificate_t native_peer_certificate;
    auto status = ::QueryContextAttributes(channel.handle_p,
      SECPKG_ATTR_REMOTE_CERT_CONTEXT,
      &native_peer_certificate.ref
    );
    if (status == SEC_E_OK)
    {
      return channel.factory->certificate_check(
        certificate_t::from_native_handle(
          std::move(native_peer_certificate)
        )
      );
    }
    error.assign(status, category());
    return false;
  }
  return true;
}


void finish_handshake (__bits::channel_t &channel, std::error_code &error)
  noexcept
{
  if (is_trusted_peer(channel, error))
  {
    LOG(std::cout << "    * connected");
    ::SecPkgContext_StreamSizes sizes{};
    auto status = ::QueryContextAttributes(channel.handle_p,
      SECPKG_ATTR_STREAM_SIZES,
      &sizes
    );
    if (status == SEC_E_OK)
    {
      channel.header_size = sizes.cbHeader;
      channel.trailer_size = sizes.cbTrailer;
      channel.max_message_size = sizes.cbMaximumMessage;
      channel.connected();
      LOG(std::cout
        << ", header=" << channel.header_size
        << ", trailer=" << channel.trailer_size
        << ", message=" << channel.max_message_size
        << "\n"
      );
    }
    else
    {
      LOG(std::cout << ", failed to get sizes\n");
      error.assign(status, category());
      channel.abort();
    }
  }
  else
  {
    LOG(std::cout << ", not trusted\n");
    channel.abort();
    error = channel.handshake_status;
  }
}


} // namespace


size_t channel_t::handshake (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  auto &channel = *impl_;
  error.clear();

  LOG(std::cout << (channel.factory->server ? "server" : "client")
    << "> handshake: " << size
    << (channel.handle_p ? ", valid" : "")
    << '\n'
  );

  size_t consumed = size, not_consumed = 0U;
  if (!channel.in.empty()
    && buffer_while_incomplete_message(channel, &data, &size, error))
  {
    return consumed;
  }

  SECURITY_STATUS status;
  uint8_t token[16 * 1024], alert[64];
  do
  {
    buffer_t in_[] =
    {
      token_t{data, size},
      empty_t{},
      extra_t{},
    };
    buffer_t out_[] =
    {
      token_t{token, sizeof(token)},
      alert_t{alert, sizeof(alert)},
    };
    buffer_t::list_t in{in_}, out{out_};

    if (channel.factory->server)
    {
      status = call(::AcceptSecurityContext,
        &channel.factory->credentials,          // phCredentials
        channel.handle_p,                       // phContext
        &in,                                    // pInput
        channel.context_request,                // fContextReq
        0,                                      // TargetDataRep
        &channel.handle,                        // phNewContext
        &out,                                   // pOutput
        &channel.context_flags,                 // pfContextAttr
        nullptr                                 // ptsTimeStamp
      );
    }
    else
    {
      status = call(::InitializeSecurityContext,
        &channel.factory->credentials,          // phCredentials
        channel.handle_p,                       // phContext
        (SEC_CHAR *)channel.peer_name.c_str(),  // pszTargetName
        channel.context_request,                // fContextReq
        0,                                      // Reserved
        0,                                      // TargetDataRep
        (channel.handle_p ? &in : nullptr),     // pInput
        0,                                      // Reserved2
        &channel.handle,                        // phNewContext
        &out,                                   // pOutput
        &channel.context_flags,                 // pfContextAttr
        nullptr                                 // ptsExpiry
      );
    }

    print_bufs("In", in);
    print_bufs("Out", out);

    switch (status)
    {
      case SEC_E_OK:
        finish_handshake(channel, error);
        [[fallthrough]];

      case SEC_I_CONTINUE_NEEDED:
      case SEC_I_MESSAGE_FRAGMENT:
        channel.handle_p = &channel.handle;
        if (handle_token(channel, out[0], buffer_manager, error))
        {
          not_consumed = handle_extra(channel, in[1]);
        }
        else
        {
          channel.abort();
        }
        break;

      case SEC_E_INCOMPLETE_MESSAGE:
        if (!handle_missing(channel, in[1], data, size, error))
        {
          channel.abort();
        }
        break;

      default:
        channel.abort();
        error.assign(status, category());
        return {};
    }
  } while (status == SEC_I_MESSAGE_FRAGMENT);

  return consumed - not_consumed;
}


void channel_t::encrypt (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  auto &channel = *impl_;
  error.clear();

  LOG(std::cout << (channel.factory->server ? "server" : "client")
    << "> encrypt: " << size
    << '\n'
  );

  uint8_t buffer[16 * 1024];
  while (size)
  {
    auto chunk_ptr = buffer;
    auto chunk_size = (std::min)({size, sizeof(buffer), channel.max_message_size});

    buffer_t io_[] =
    {
      header_t{chunk_ptr, channel.header_size},
      data_t{chunk_ptr += channel.header_size, chunk_size},
      trailer_t{chunk_ptr += chunk_size, channel.trailer_size},
      empty_t{},
    };
    buffer_t::list_t io{io_};

    chunk_ptr = buffer + channel.header_size;
    std::uninitialized_copy_n(data, chunk_size,
      sal::__bits::make_output_iterator(chunk_ptr, chunk_ptr + chunk_size)
    );

    auto status = call(::EncryptMessage, channel.handle_p, 0, &io, 0);
    print_bufs("IO", io);

    if (status == SEC_E_OK)
    {
      if (!flush(channel,
          buffer_manager,
          buffer,
          channel.header_size + chunk_size + channel.trailer_size,
          error))
      {
        return;
      }
    }
    else
    {
      error.assign(status, category());
      return;
    }

    data += chunk_size;
    size -= chunk_size;
  }
}


size_t channel_t::decrypt (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  auto &channel = *impl_;
  error.clear();

  LOG(std::cout << (channel.factory->server ? "server" : "client")
    << "> decrypt: " << size
    << '\n'
  );

  size_t consumed = size, not_consumed = 0U;
  if (!channel.in.empty()
    && buffer_while_incomplete_message(channel, &data, &size, error))
  {
    return consumed;
  }

  buffer_t io_[] =
  {
    data_t{data, size},
    empty_t{},
    empty_t{},
    empty_t{},
  };
  buffer_t::list_t io{io_};

  auto status = call(::DecryptMessage, channel.handle_p, &io, 0, nullptr);
  print_bufs("IO", io);

  switch (status)
  {
    case SEC_E_OK:
      if (handle_data(channel, io[1], buffer_manager, error))
      {
        not_consumed = handle_extra(channel, io[3]);
      }
      break;

    case SEC_E_INCOMPLETE_MESSAGE:
      handle_missing(channel, io[1], data, size, error);
      break;

    default:
      error.assign(status, category());
      break;
  }

  return consumed - not_consumed;
}


#endif //}}}1


} // namespace crypto


__sal_end
