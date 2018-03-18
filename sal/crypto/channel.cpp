#include <sal/crypto/channel.hpp>

#if __sal_os_linux //{{{1
  #include <sal/byte_order.hpp>
  #include <mutex>
  #include <openssl/bio.h>
  #include <openssl/err.h>
  #include <openssl/x509v3.h>
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


// because TLS/DTLS IO is based on buffer_manager, it is actually application
// responsibility to provide properly sized buffers.
// This value is used as default MTU size for SSL_set_mtu()
constexpr const size_t mtu = 1472;


#if OPENSSL_VERSION_NUMBER < 0x10100000

inline constexpr const bool openssl_pre_1_1 = true;


inline void BIO_set_init (BIO *bio, int init) noexcept
{
  bio->init = init;
}


inline void BIO_set_data (BIO *bio, void *data) noexcept
{
  bio->ptr = data;
}


inline void *BIO_get_data (BIO *bio) noexcept
{
  return bio->ptr;
}


inline auto TLS_client_method () noexcept
{
  return TLSv1_client_method();
}


inline auto TLS_server_method () noexcept
{
  return TLSv1_server_method();
}


inline auto DTLS_client_method () noexcept
{
  return DTLSv1_client_method();
}


inline auto DTLS_server_method () noexcept
{
  return DTLSv1_server_method();
}


inline STACK_OF(X509) *X509_STORE_CTX_get0_untrusted (X509_STORE_CTX *ctx)
  noexcept
{
  return ctx->untrusted;
}


#else

inline constexpr const bool openssl_pre_1_1 = false;

#endif


struct bio_t
{
  static constexpr const char name[] = "buffer_manager";
  static BIO_METHOD *methods;

  static int create (BIO *bio) noexcept;
  static long ctrl (BIO *bio, int cmd, long num, void *ptr) noexcept;
  static int write (BIO *bio, const char *data, int size) noexcept;
  static int read (BIO *bio, char *data, int size) noexcept;

  struct io_t
  {
    std::error_code &error;
    channel_t::buffer_manager_t &buffer_manager;
    const uint8_t *ptr, *end;
  };
};
BIO_METHOD *bio_t::methods = nullptr;


int bio_t::create (BIO *bio) noexcept
{
  BIO_set_init(bio, 1);
  return 1;
}


long bio_t::ctrl (BIO *, int cmd, long, void *) noexcept
{
  switch (cmd)
  {
    case BIO_CTRL_PENDING:
    case BIO_CTRL_WPENDING:
      return 0;

    case BIO_CTRL_DGRAM_MTU_EXCEEDED:
      return 0;

#if defined(BIO_CTRL_DGRAM_GET_MTU_OVERHEAD)
    case BIO_CTRL_DGRAM_GET_MTU_OVERHEAD:
      // (20 IPv4 | 40 IPv6) header + 8 UDP header
      // here we don't know which, go with IPv4
      return 28;
#endif
  }

  return 1;
}


int bio_t::write (BIO *bio, const char *data, int size) noexcept
{
  LOG(std::cout << "    | write " << size);

  auto &io = *static_cast<io_t *>(BIO_get_data(bio));

  BIO_clear_retry_flags(bio);

  int written = 0;
  while (size)
  {
    uint8_t *chunk_ptr{};
    size_t chunk_size{};
    auto user_data = io.buffer_manager.alloc(&chunk_ptr, &chunk_size);
    if (chunk_ptr && chunk_size)
    {
      if (chunk_size > static_cast<size_t>(size))
      {
        chunk_size = size;
      }
      std::uninitialized_copy_n(data, chunk_size, chunk_ptr);
      io.buffer_manager.ready(user_data, chunk_ptr, chunk_size);
      written += chunk_size;
      data += chunk_size;
      size -= chunk_size;
    }
    else
    {
      LOG(std::cout << ", no buffer space\n");
      io.error = std::make_error_code(std::errc::no_buffer_space);
      return -1;
    }
  }

  LOG(std::cout << ", succeeded " << written << '\n');
  return written;
}


int bio_t::read (BIO *bio, char *data, int size) noexcept
{
  LOG(std::cout << "    | read " << size);

  auto &io = *static_cast<io_t *>(BIO_get_data(bio));

  if (io.ptr < io.end)
  {
    if (io.ptr + size > io.end)
    {
      size = io.end - io.ptr;
    }
    std::uninitialized_copy_n(io.ptr, size,
      reinterpret_cast<uint8_t *>(data)
    );
    io.ptr += size;
    BIO_clear_retry_flags(bio);
  }
  else
  {
    size = -1;
    BIO_set_retry_read(bio);
  }

  LOG(std::cout << ", got " << size << '\n');
  return size;
}


inline void init_openssl () noexcept
{
#if OPENSSL_VERSION_NUMBER < 0x10100000

  SSL_library_init();
  SSL_load_error_strings();

  static BIO_METHOD bio_methods =
  {
    BIO_TYPE_SOURCE_SINK,
    bio_t::name,
    &bio_t::write,
    &bio_t::read,
    nullptr,
    nullptr,
    &bio_t::ctrl,
    &bio_t::create,
    nullptr,
    nullptr
  };
  bio_t::methods = &bio_methods;

#else

  // will be leaked but don't care
  bio_t::methods = BIO_meth_new(
    BIO_TYPE_SOURCE_SINK | BIO_get_new_index(),
    bio_t::name
  );
  if (bio_t::methods)
  {
    BIO_meth_set_create(bio_t::methods, &bio_t::create);
    BIO_meth_set_ctrl(bio_t::methods, &bio_t::ctrl);
    BIO_meth_set_write(bio_t::methods, &bio_t::write);
    BIO_meth_set_read(bio_t::methods, &bio_t::read);
  }

#endif
}


inline const SSL_METHOD *channel_type (bool datagram, bool server) noexcept
{
  static const SSL_METHOD * const method[] =
  {
    TLS_client_method(),
    TLS_server_method(),
    DTLS_client_method(),
    DTLS_server_method(),
  };
  return method[datagram * 2 + server];
}


inline const char *ca_path () noexcept
{
  if (auto path = std::getenv(X509_get_default_cert_dir_env()))
  {
    return path;
  }
  return X509_get_default_cert_dir();
}


bool set_chain (__bits::channel_factory_t &factory,
  std::error_code &error) noexcept
{
  auto result = SSL_CTX_load_verify_locations(factory.handle.ref,
    nullptr,
    ca_path()
  );
  if (result != 1)
  {
    error.assign(ERR_get_error(), category());
    return false;
  }

  if (factory.chain.size())
  {
    result = SSL_CTX_use_certificate(factory.handle.ref,
      factory.chain[0].native_handle().ref
    );
    if (result != 1)
    {
      error.assign(ERR_get_error(), category());
      return false;
    }

    size_t it = 1U;
    while (it < factory.chain.size()
      && !factory.chain[it].is_self_signed())
    {
#if OPENSSL_VERSION_NUMBER < 0x10002000L
      __bits::certificate_t native_cert = factory.chain[it].native_handle();
      __bits::inc_ref(native_cert.ref);
      SSL_CTX_add_extra_chain_cert(factory.handle.ref, native_cert.ref);
#else
      SSL_CTX_add1_chain_cert(factory.handle.ref,
        factory.chain[it].native_handle().ref
      );
#endif
      it++;
    }

    result = SSL_CTX_use_PrivateKey(factory.handle.ref, factory.private_key);
    if (result != 1)
    {
      error.assign(ERR_get_error(), category());
      return false;
    }

    result = SSL_CTX_check_private_key(factory.handle.ref);
    if (result != 1)
    {
      error.assign(ERR_get_error(), category());
      return false;
    }
  }

  return true;
}


inline int channel_index () noexcept
{
  static const int index = SSL_get_ex_new_index(0,
    (void *)"channel",
    nullptr,
    nullptr,
    nullptr
  );
  return index;
}


int manual_certificate_check (X509_STORE_CTX *ctx, void *arg) noexcept
{
  auto list = X509_STORE_CTX_get0_untrusted(ctx);
  std::vector<certificate_t> chain;
  chain.reserve(sk_X509_num(list));

  while (chain.size() < (size_t)sk_X509_num(list))
  {
    __bits::certificate_t native_cert = sk_X509_value(list, chain.size());
    __bits::inc_ref(native_cert.ref);
    chain.emplace_back(
      certificate_t::from_native_handle(std::move(native_cert))
    );
  }

  auto &factory = *static_cast<__bits::channel_factory_t *>(arg);
  if (factory.chain_check(chain))
  {
    LOG(std::cout << "    | certificate: accept\n");
    return 1;
  }

  X509_STORE_CTX_set_error(ctx, X509_V_ERR_CERT_REJECTED);
  LOG(std::cout << "    | certificate: reject\n");
  return 0;
}


void setup_manual_verification (__bits::channel_factory_t &factory) noexcept
{
  if (factory.chain_check)
  {
    SSL_CTX_set_cert_verify_callback(factory.handle.ref,
      manual_certificate_check,
      &factory
    );
  }
}


#if WITH_LOGGING
int certificate_post_check (int preverify_ok, X509_STORE_CTX *ctx) noexcept
{
  auto certificate = X509_STORE_CTX_get_current_cert(ctx);
  auto error = X509_STORE_CTX_get_error(ctx);

  char subject[256];
  X509_NAME_oneline(X509_get_subject_name(certificate), subject, sizeof(subject));

  LOG(std::cout << "    | cert " << subject
    << " ("
    << (preverify_ok ? "ok" : X509_verify_cert_error_string(error))
    << ")\n"
  );

  return preverify_ok;
}
#endif


void setup_verification (__bits::channel_t &channel) noexcept
{
  int mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
  if (channel.factory->server && !channel.mutual_auth)
  {
    mode = SSL_VERIFY_NONE;
  }

  auto callback =
    #if WITH_LOGGING
      &certificate_post_check
    #else
      nullptr
    #endif
  ;

  if (!channel.peer_name.empty())
  {
#if OPENSSL_VERSION_NUMBER < 0x10100000
    // TODO
    // <1.0.2: https://wiki.openssl.org/index.php/Hostname_validation
    // 1.0.2: X509_check_host()
#else
    auto x509_check_args = SSL_get0_param(channel.handle.ref);
    X509_VERIFY_PARAM_set_hostflags(x509_check_args,
      X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS
    );
    X509_VERIFY_PARAM_set1_host(x509_check_args,
      channel.peer_name.c_str(),
      channel.peer_name.size()
    );
#endif
  }

  SSL_set_verify(channel.handle.ref, mode, callback);
}


bool disable_read_ahead (__bits::channel_factory_t &factory, std::error_code &)
  noexcept
{
  SSL_CTX_set_read_ahead(factory.handle.ref, 0);
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
          std::cout << "    > " #w " " << SSL_state_string_long(ssl) << '\n'; \
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
    std::cout << "    > error " << ret << '\n';
  }
}
#endif


inline size_t dtls_record_size (const uint8_t *data, size_t size) noexcept
{
  if (size < 13)
  {
    return size;
  }

  // see https://tools.ietf.org/html/rfc4347#section-4.3.1
  uint16_t claimed_size = 13 + network_to_native_byte_order(
    *reinterpret_cast<const uint16_t *>(data + 11)
  );
  return claimed_size > size ? size : claimed_size;
}


} // namespace


void __bits::channel_factory_t::ctor (std::error_code &error) noexcept
{
  static std::once_flag init_flag;
  std::call_once(init_flag, init_openssl);

  if (!bio_t::methods)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return;
  }

  handle = SSL_CTX_new(channel_type(datagram, server));
  if (handle)
  {
    if (set_chain(*this, error)
      && disable_read_ahead(*this, error))
    {
      setup_manual_verification(*this);
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
  handle = SSL_new(factory->handle.ref);
  if (handle)
  {
    if (factory->server)
    {
      SSL_set_accept_state(handle.ref);
    }
    else
    {
      SSL_set_connect_state(handle.ref);
    }
    LOG(SSL_set_info_callback(handle.ref, info_callback));

    SSL_set_ex_data(handle.ref, channel_index(), this);
    setup_verification(*this);

    bio = BIO_new(bio_t::methods);
    if (bio)
    {
      if (factory->datagram)
      {
        SSL_set_options(handle.ref, SSL_OP_NO_QUERY_MTU);
        SSL_set_mtu(handle.ref, mtu);
      }
      SSL_set_bio(handle.ref, bio, bio);
      error.clear();
    }
    else
    {
      error.assign(ERR_get_error(), category());
    }
  }
  else
  {
    error = std::make_error_code(std::errc::not_enough_memory);
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

  bio_t::io_t io = { error, buffer_manager, data, data + size };
  BIO_set_data(channel.bio, &io);

  ERR_clear_error();
  error.clear();

  auto status = SSL_do_handshake(channel.handle.ref);
  switch (SSL_get_error(channel.handle.ref, status))
  {
    case SSL_ERROR_NONE:
      LOG(std::cout << "    | connected\n");
      channel.connected();
      break;

    case SSL_ERROR_SSL:
    case SSL_ERROR_SYSCALL:
      channel.aborted();
      if (!error)
      {
        // assign only if not already set by bio_t::io_t
        error.assign(ERR_get_error(), category());
      }
      LOG(std::cout << "    | error: " << error.message() << "\n");
      return {};
  }

  return io.ptr - data;
}


void channel_t::encrypt (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  auto &channel = *impl_;

  LOG(std::cout << (channel.factory->server ? "server" : "client")
    << "> encrypt: " << size << '\n'
  );

  bio_t::io_t io = { error, buffer_manager, nullptr, nullptr };
  BIO_set_data(channel.bio, &io);

  ERR_clear_error();
  error.clear();

  auto status = SSL_write(channel.handle.ref, data, size);
  switch (SSL_get_error(channel.handle.ref, status))
  {
    case SSL_ERROR_NONE:
      return;

    case SSL_ERROR_SSL:
    case SSL_ERROR_SYSCALL:
      if (!error)
      {
        error.assign(ERR_get_error(), category());
      }
      LOG(std::cout << "    | error: " << error.message() << '\n');
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

  if (channel.factory->datagram)
  {
    // OpenSSL DTLS correctly consumes all datagram payload but to unify with
    // other platforms' implementations, give it only single TLS record and
    // let application to call again with remaining data
    size = dtls_record_size(data, size);
  }

  bio_t::io_t io = { error, buffer_manager, data, data + size };
  BIO_set_data(channel.bio, &io);

  ERR_clear_error();
  error.clear();

  for (;;)
  {
    size_t chunk_size{};
    uint8_t *chunk_ptr;
    auto user_data = buffer_manager.alloc(&chunk_ptr, &chunk_size);
    if (!chunk_ptr || !chunk_size)
    {
      error = std::make_error_code(std::errc::no_buffer_space);
      break;
    }

    auto status = SSL_read(channel.handle.ref, chunk_ptr, chunk_size);
    switch (SSL_get_error(channel.handle.ref, status))
    {
      case SSL_ERROR_NONE:
        buffer_manager.ready(user_data, chunk_ptr, status);
        if (SSL_pending(channel.handle.ref)
          || (!openssl_pre_1_1 && channel.factory->datagram))
        {
          // XXX: OpenSSL-1.1 SSL_pending returns 0 for DTLS
          // keep reading until SSL_ERROR_WANT_READ is returned
          continue;
        }
        break;

      case SSL_ERROR_SSL:
      case SSL_ERROR_SYSCALL:
        buffer_manager.ready(user_data, chunk_ptr, 0U);
        if (!error)
        {
          error.assign(ERR_get_error(), category());
        }
        LOG(std::cout << "    | error: " << error.message() << '\n');
        break;

      default:
        LOG(std::cout << "    | unhandled " << status << '\n');
        buffer_manager.ready(user_data, chunk_ptr, 0U);
        break;
    }
    break;
  }

  return io.ptr - data;
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


bool set_chain (__bits::channel_t &channel, std::error_code &error) noexcept
{
  if (!channel.factory->chain.empty())
  {
    unique_ref<::SecIdentityRef> identity;
    auto status = ::SecIdentityCreateWithCertificate(nullptr,
      channel.factory->chain[0].native_handle().ref,
      &identity.ref
    );
    if (status != ::errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }

    constexpr const size_t max_chain_size = 9;
    CFTypeRef chain[max_chain_size] = { identity.ref };
    size_t chain_size = 1;
    while (chain_size < max_chain_size
      && chain_size < channel.factory->chain.size()
      && !channel.factory->chain[chain_size].is_self_signed())
    {
      chain[chain_size] = channel.factory->chain[chain_size].native_handle().ref;
      chain_size++;
    }

    unique_ref<::CFArrayRef> trust = ::CFArrayCreate(nullptr,
      chain, chain_size,
      &::kCFTypeArrayCallBacks
    );
    if (!trust.ref)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
      return false;
    }

    status = ::SSLSetCertificate(channel.handle.ref, trust.ref);
    if (status != ::errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool set_chain_check (__bits::channel_t &channel, std::error_code &error)
  noexcept
{
  if (channel.factory->chain_check)
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


bool peer_auth (__bits::channel_t &channel, std::error_code &error)
  noexcept
{
  unique_ref<::SecTrustRef> trust{};
  auto status = ::SSLCopyPeerTrust(channel.handle.ref, &trust.ref);
  if (status != ::errSecSuccess)
  {
    error.assign(status, category());
    return false;
  }

  size_t chain_size = ::SecTrustGetCertificateCount(trust.ref);
  std::vector<certificate_t> chain;
  chain.reserve(chain_size);

  while (chain.size() < chain_size)
  {
    auto handle = ::SecTrustGetCertificateAtIndex(trust.ref, chain.size());
    ::CFRetain(handle);
    chain.emplace_back(certificate_t::from_native_handle(handle));
  }

  if (chain.size() && channel.factory->chain_check(chain))
  {
    return true;
  }

  error.assign(errSSLPeerHandshakeFail, category());
  return false;
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
    && set_chain(*this, error)
    && set_chain_check(*this, error))
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
  error.clear();

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
        break;

      case ::errSSLWouldBlock:
        LOG(std::cout << "    | blocked\n");
        break;

      case ::errSSLBufferOverflow:
        LOG(std::cout << "    | overflow\n");
        error = std::make_error_code(std::errc::no_buffer_space);
        break;

      case ::errSSLPeerAuthCompleted:
        LOG(std::cout << "    | peer_auth\n");
        if (!peer_auth(channel, error))
        {
          channel.aborted();
          break;
        }
        continue;

      default:
        LOG(std::cout << "    | error\n");
        channel.aborted();
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
      error.assign(status, category());
      LOG(std::cout << "    | error: " << error.message() << "\n");
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
  if (chain_check)
  {
    auth_data.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;
  }

  PCCERT_CONTEXT chain_data[1];
  if (chain.size())
  {
    // SChannel builds chain itself
    chain_data[0] = chain[0].native_handle().ref;
    auth_data.paCred = chain_data;
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


std::vector<certificate_t> to_chain (HCERTSTORE store)
{
  PCCERT_CONTEXT it = nullptr;
  std::vector<certificate_t> chain;
  while ((it = ::CertEnumCertificatesInStore(store, it)) != nullptr)
  {
    chain.push_back(
      certificate_t::from_native_handle(
        ::CertDuplicateCertificateContext(it)
      )
    );
  }
  std::reverse(chain.begin(), chain.end());
  return chain;
}


bool peer_auth (__bits::channel_t &channel, std::error_code &error)
  noexcept
{
  if (channel.factory->chain_check)
  {
    __bits::certificate_t native_peer_certificate;
    auto status = ::QueryContextAttributes(channel.handle_p,
      SECPKG_ATTR_REMOTE_CERT_CONTEXT,
      &native_peer_certificate.ref
    );
    if (status == SEC_E_OK)
    {
      return channel.factory->chain_check(
        to_chain(native_peer_certificate.ref->hCertStore)
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
  if (peer_auth(channel, error))
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
      channel.aborted();
    }
  }
  else
  {
    LOG(std::cout << "    * reject\n");
    error.assign(SEC_E_CERT_UNKNOWN, category());
    channel.aborted();
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
        if (error)
        {
          return {};
        }
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
          channel.aborted();
        }
        break;

      case SEC_E_INCOMPLETE_MESSAGE:
        if (!handle_missing(channel, in[1], data, size, error))
        {
          channel.aborted();
        }
        break;

      default:
        channel.aborted();
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
      auto buffer_size = channel.header_size + chunk_size + channel.trailer_size;
      if (!flush(channel, buffer_manager, buffer, buffer_size, error))
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
