#include <sal/crypto/__bits/pipe.hpp>

#if __sal_os_linux
  #include <mutex>
  #include <openssl/err.h>
  #include <openssl/opensslv.h>
  #include <openssl/ssl.h>
#elif __sal_os_macos
  #include <Security/SecIdentity.h>
#elif __sal_os_windows
  #include <sal/memory.hpp>
  #include <schannel.h>
  #include <security.h>
  #pragma comment(lib, "secur32")
#endif


#define WITH_LOGGING 0
#if WITH_LOGGING
  #include <iostream>
  #include <sstream>
  #define LOG(x) x
#else
  #define LOG(x)
#endif


__sal_begin


namespace crypto::__bits {


#if __sal_os_linux //{{{1


namespace {


inline void init_openssl () noexcept
{
#if OPENSSL_VERSION_NUMBER < 0x10100000
  static std::once_flag init_flag;
  std::call_once(init_flag,
    []()
    {
      ::SSL_library_init();
      ::SSL_load_error_strings();
    }
  );
#endif
}


inline const SSL_METHOD *tls_method (bool server, bool datagram) noexcept
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


bool set_certificate (pipe_factory_t &factory, std::error_code &error) noexcept
{
  if (factory.certificate.ref)
  {
    auto result = ::SSL_CTX_use_certificate(
      factory.context.ref,
      factory.certificate.ref
    );
    if (result != 1)
    {
      error.assign(::ERR_get_error(), category());
      return false;
    }

    result = ::SSL_CTX_use_PrivateKey(
      factory.context.ref,
      factory.private_key
    );
    if (result != 1)
    {
      error.assign(::ERR_get_error(), category());
      return false;
    }

    result = ::SSL_CTX_check_private_key(factory.context.ref);
    if (result != 1)
    {
      error.assign(::ERR_get_error(), category());
      return false;
    }
  }
  return true;
}


#if WITH_LOGGING
void info_callback(const SSL* ssl, int where, int ret)
{
  if (ret != 0)
  {
    #define I_(w) \
      do { if (where & SSL_CB_ ## w) { \
        printf("    | %15.15s - %s\n", #w, ::SSL_state_string_long(ssl)); \
      }} while (false)
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
    printf("error, ret=%d!\n", ret);
  }
}
#endif


bool ssl_read (pipe_t &pipe, std::error_code &error) noexcept
{
  auto size = pipe.in_last - pipe.in_ptr;
  if (size > 0)
  {
    size = ::BIO_write(pipe.in, pipe.in_ptr, size);
    if (size > 0)
    {
      LOG(std::cout << "    BIO_write=" << size << '\n');
      pipe.in_ptr += size;
    }
    else if (!::BIO_should_retry(pipe.in))
    {
      LOG(std::cout << "    BIO_write: no retry\n");
      error.assign(::ERR_get_error(), category());
      return false;
    }
    else
    {
      LOG(std::cout << "    BIO_write: retry?\n");
    }
  }
  return true;
}


bool ssl_write (pipe_t &pipe, std::error_code &error) noexcept
{
  auto size = pipe.out_last - pipe.out_ptr;
  if (size > 0)
  {
    size = ::BIO_read(pipe.out, pipe.out_ptr, size);
    if (size > 0)
    {
      LOG(std::cout << "    BIO_read=" << size << '\n');
      pipe.out_ptr += size;
    }
    else if (!::BIO_should_retry(pipe.out))
    {
      LOG(std::cout << "    BIO_read: no retry\n");
      error.assign(::ERR_get_error(), category());
      return false;
    }
    else
    {
      LOG(std::cout << "    BIO_read: retry?\n");
    }
  }
  return true;
}


} // namespace


void pipe_t::ctor (std::error_code &error) noexcept
{
  context = ::SSL_new(factory->context.ref);
  if (context)
  {
    if (factory->server)
    {
      side = 'S';
      ::SSL_set_accept_state(context.ref);
    }
    else
    {
      side = 'C';
      ::SSL_set_connect_state(context.ref);
    }
    LOG(::SSL_set_info_callback(context.ref, info_callback));

    in = ::BIO_new(::BIO_s_mem());
    out = ::BIO_new(::BIO_s_mem());

    if (in && out)
    {
      ::BIO_set_mem_eof_return(in, -1);
      ::BIO_set_mem_eof_return(out, -1);
      ::SSL_set_bio(context.ref, in, out);
      error.clear();
      return;
    }

    ::BIO_free_all(in);
    ::BIO_free_all(out);
  }
  error = std::make_error_code(std::errc::not_enough_memory);
}


pipe_t::~pipe_t () noexcept
{ }


std::pair<size_t, size_t> pipe_t::handshake (std::error_code &error)
{
  LOG(std::cout << side
    << "> handshake, IN: " << (in_last - in_ptr)
    << ", OUT: " << (out_last - out_ptr) << '\n');

  if (!ssl_read(*this, error))
  {
    return {};
  }

  auto status = ::SSL_do_handshake(context.ref);
  switch (::SSL_get_error(context.ref, status))
  {
    case SSL_ERROR_NONE:
      handshake_result = std::make_error_code(std::errc::already_connected);
      if (!::BIO_ctrl_pending(out))
      {
        error.clear();
        break;
      }
      [[fallthrough]];

    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      if (ssl_write(*this, error))
      {
        error.clear();
      }
      break;

    case SSL_ERROR_ZERO_RETURN:
      LOG(std::cout << "    aborted\n");
      handshake_result = std::make_error_code(std::errc::connection_aborted);
      error.clear();
      break;

    case SSL_ERROR_SSL:
    case SSL_ERROR_SYSCALL:
      LOG(std::cout << "    error\n");
      handshake_result = std::make_error_code(std::errc::connection_aborted);
      error.assign(::ERR_get_error(), category());
      break;

    default:
      LOG(std::cout << "    unhandled state\n");
      break;
  }

  LOG(std::cout
    << "    < IN: " << (in_last - in_ptr)
    << ", OUT: " << (out_ptr - out_first) << '\n');

  return {in_ptr - in_first, out_ptr - out_first};
}


std::pair<size_t, size_t> pipe_t::encrypt (std::error_code &error)
{
  error.clear();
  return {};
}


std::pair<size_t, size_t> pipe_t::decrypt (std::error_code &error)
{
  error.clear();
  return {};
}


void pipe_factory_t::ctor (std::error_code &error) noexcept
{
  init_openssl();

  context = ::SSL_CTX_new(tls_method(server, datagram));
  if (context)
  {
    if (set_certificate(*this, error))
    {
      error.clear();
    }
  }
  else
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
}


pipe_factory_t::~pipe_factory_t () noexcept
{ }


#elif __sal_os_macos //{{{1


namespace {


inline pipe_t &to_pipe (::SSLConnectionRef connection) noexcept
{
  return *const_cast<pipe_t *>(static_cast<const pipe_t *>(connection));
}


OSStatus pipe_read (::SSLConnectionRef connection, void *data, size_t *size)
  noexcept
{
  LOG(std::cout << "    read: " << *size);
  auto &pipe = to_pipe(connection);
  if (pipe.in_ptr < pipe.in_last)
  {
    OSStatus status = errSecSuccess;
    size_t have = pipe.in_last - pipe.in_ptr;
    if (have < *size)
    {
      *size = have;
      status = errSSLWouldBlock;
      LOG(std::cout << ", block: " << have << '\n');
    }
    else
    {
      LOG(std::cout << ", ok\n");
    }
    std::uninitialized_copy_n(
      pipe.in_ptr, *size, static_cast<uint8_t *>(data)
    );
    pipe.in_ptr += *size;
    return status;
  }
  LOG(std::cout << ", block: none\n");
  *size = 0;
  return errSSLWouldBlock;
}


OSStatus pipe_write (::SSLConnectionRef connection, const void *data, size_t *size)
  noexcept
{
  LOG(std::cout << "    write: " << *size);
  auto &pipe = to_pipe(connection);
  if (pipe.out_ptr < pipe.out_last)
  {
    OSStatus status = errSecSuccess;
    size_t room = pipe.out_last - pipe.out_ptr;
    if (*size > room)
    {
      *size = room;
      status = errSSLWouldBlock;
      LOG(std::cout << ", block: " << room << '\n');
    }
    else
    {
      LOG(std::cout << ", ok\n");
    }
    pipe.out_ptr = std::uninitialized_copy_n(
      static_cast<const uint8_t *>(data), *size, pipe.out_ptr
    );
    return status;
  }
  LOG(std::cout << ", none\n");
  *size = 0;
  return errSSLWouldBlock;
}


bool set_io (pipe_t &pipe, std::error_code &error) noexcept
{
  auto status = ::SSLSetIOFuncs(pipe.context.ref, &pipe_read, &pipe_write);
  if (status == errSecSuccess)
  {
    return true;
  }
  error.assign(status, category());
  return false;
}


bool set_connection (pipe_t &pipe, std::error_code &error) noexcept
{
  auto status = ::SSLSetConnection(pipe.context.ref, &pipe);
  if (status == errSecSuccess)
  {
    return true;
  }
  error.assign(status, category());
  return false;
}


bool set_peer_name (pipe_t &pipe, std::error_code &error) noexcept
{
  if (!pipe.peer_name.empty())
  {
    auto status = ::SSLSetPeerDomainName(pipe.context.ref,
      pipe.peer_name.data(),
      pipe.peer_name.size()
    );
    if (status != errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool set_certificate (pipe_t &pipe, std::error_code &error) noexcept
{
  if (pipe.factory->certificate)
  {
    unique_ref<SecIdentityRef> identity;
    auto status = ::SecIdentityCreateWithCertificate(nullptr,
      pipe.factory->certificate.ref,
      &identity.ref
    );
    if (status != errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }

    unique_ref<CFArrayRef> certificates = ::CFArrayCreate(nullptr,
      (CFTypeRef *)&identity.ref, 1,
      &kCFTypeArrayCallBacks
    );
    if (!certificates.ref)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
      return false;
    }

    status = ::SSLSetCertificate(pipe.context.ref, certificates.ref);
    if (status != errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool set_mutual_auth (pipe_t &pipe, std::error_code &error) noexcept
{
  if (pipe.factory->mutual_auth)
  {
    auto status = ::SSLSetClientSideAuthenticate(pipe.context.ref,
      kAlwaysAuthenticate
    );
    if (status != errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool set_certificate_check (pipe_t &pipe, std::error_code &error) noexcept
{
  if (pipe.factory->certificate_check)
  {
    auto break_on_auth = pipe.factory->server
      ? kSSLSessionOptionBreakOnClientAuth
      : kSSLSessionOptionBreakOnServerAuth
    ;
    auto status = ::SSLSetSessionOption(pipe.context.ref, break_on_auth, true);
    if (status != errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool trusted_peer (const pipe_t &pipe, std::error_code &error) noexcept
{
  unique_ref<SecTrustRef> trust{};
  auto status = ::SSLCopyPeerTrust(pipe.context.ref, &trust.ref);
  if (status != errSecSuccess)
  {
    error.assign(status, category());
    return false;
  }

  auto peer_certificate = crypto::certificate_t::from_native_handle(
    ::SecTrustGetCertificateAtIndex(trust.ref, 0)
  );
  ::CFRetain(peer_certificate.native_handle().ref);

  return pipe.factory->certificate_check(peer_certificate);
}


} // namespace


void pipe_t::ctor (std::error_code &error) noexcept
{
  context.ref = ::SSLCreateContext(nullptr,
    factory->server ? kSSLServerSide : kSSLClientSide,
    factory->datagram ? kSSLDatagramType : kSSLStreamType
  );
  if (!context)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return;
  }

  side = factory->server ? 'S' : 'C';
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


pipe_t::~pipe_t () noexcept
{ }


std::pair<size_t, size_t> pipe_t::handshake (std::error_code &error)
{
  LOG(std::cout << side
    << "> handshake, IN: " << (in_last - in_ptr)
    << ", OUT: " << (out_last - out_ptr) << '\n');

  auto status = ::SSLHandshake(context.ref);

  LOG(std::cout
    << "    < IN: " << (in_last - in_ptr)
    << ", OUT: " << (out_ptr - out_first) << '\n');

  if (status == errSecSuccess)
  {
    LOG(std::cout << side << "> connected\n");
    handshake_result = std::make_error_code(std::errc::already_connected);
    error.clear();
  }
  else if (status == errSSLWouldBlock)
  {
    LOG(std::cout << side << "> blocked\n");
    error.clear();
  }
  else if (status == errSSLPeerAuthCompleted)
  {
    LOG(std::cout << side << "> certificate_check\n");
    if (trusted_peer(*this, error))
    {
      handshake(error);
    }
    else
    {
      handshake_result = std::make_error_code(std::errc::connection_aborted);
    }
  }
  else if (status == errSSLRecordOverflow)
  {
    LOG(std::cout << side << "> overflow\n");
    handshake_result = std::make_error_code(std::errc::no_buffer_space);
    error = handshake_result;
  }
  else
  {
    handshake_result = std::make_error_code(std::errc::connection_aborted);
    error.assign(status, category());
    LOG(std::cout << side << "> error\n");
  }

  return {in_ptr - in_first, out_ptr - out_first};
}


std::pair<size_t, size_t> pipe_t::encrypt (std::error_code &error)
{
  LOG(std::cout << side
    << "> encrypt, IN: " << (in_last - in_ptr)
    << ", OUT: " << (out_last - out_ptr) << '\n');

  size_t processed = 0U;
  auto status = ::SSLWrite(context.ref,
    in_ptr,
    (in_last - in_ptr),
    &processed
  );
  in_ptr += processed;

  LOG(std::cout
    << "    < IN: " << (in_last - in_ptr)
    << ", OUT: " << (out_ptr - out_first) << '\n');

  if (status == errSecSuccess)
  {
    LOG(std::cout << side << "> ok\n");
    error.clear();
  }
  else
  {
    error.assign(status, category());
    LOG(std::cout << side << "> error\n");
  }

  return {in_ptr - in_first, out_ptr - out_first};
}


std::pair<size_t, size_t> pipe_t::decrypt (std::error_code &error)
{
  LOG(std::cout << side
    << "> decrypt, IN: " << (in_last - in_ptr)
    << ", OUT: " << (out_last - out_ptr) << '\n');

  size_t processed = 0U;
  auto status = ::SSLRead(context.ref,
    out_ptr,
    (out_last - out_ptr),
    &processed
  );
  out_ptr += processed;

  LOG(std::cout
    << "    < IN: " << (in_last - in_ptr)
    << ", OUT: " << (out_ptr - out_first) << '\n');

  if (status == errSecSuccess)
  {
    LOG(std::cout << side << "> ok\n");
    error.clear();
  }
  else if (status == errSSLWouldBlock)
  {
    LOG(std::cout << side << "> blocked\n");
    error.clear();
  }
  else if (status == errSSLRecordOverflow)
  {
    LOG(std::cout << side << "> overflow\n");
    error = std::make_error_code(std::errc::no_buffer_space);
  }
  else
  {
    error.assign(status, category());
    LOG(std::cout << side << "> error\n");
  }

  return {in_ptr - in_first, out_ptr - out_first};
}


void pipe_factory_t::ctor (std::error_code &error) noexcept
{
  error.clear();
}


pipe_factory_t::~pipe_factory_t () noexcept
{ }


#elif __sal_os_windows //{{{1


namespace {


inline auto handle_result (SECURITY_STATUS status, const char *func)
{
#if WITH_LOGGING //{{{2
  std::ostringstream oss;
  oss << "    - " << func << ": ";
  #define S_(s) case s: oss << #s; break
  switch (status)
  {
    S_(SEC_E_OK);
    S_(SEC_E_ALGORITHM_MISMATCH);
    S_(SEC_E_APPLICATION_PROTOCOL_MISMATCH);
    S_(SEC_E_BUFFER_TOO_SMALL);
    S_(SEC_E_CERT_EXPIRED);
    S_(SEC_E_CERT_UNKNOWN);
    S_(SEC_E_DECRYPT_FAILURE);
    S_(SEC_E_ENCRYPT_FAILURE);
    S_(SEC_E_ILLEGAL_MESSAGE);
    S_(SEC_E_INCOMPLETE_CREDENTIALS);
    S_(SEC_E_INCOMPLETE_MESSAGE);
    S_(SEC_E_INSUFFICIENT_MEMORY);
    S_(SEC_E_INTERNAL_ERROR);
    S_(SEC_E_INVALID_HANDLE);
    S_(SEC_E_INVALID_PARAMETER);
    S_(SEC_E_INVALID_TOKEN);
    S_(SEC_E_LOGON_DENIED);
    S_(SEC_E_MESSAGE_ALTERED);
    S_(SEC_E_MUTUAL_AUTH_FAILED);
    S_(SEC_E_NO_AUTHENTICATING_AUTHORITY);
    S_(SEC_E_NO_CREDENTIALS);
    S_(SEC_E_OUT_OF_SEQUENCE);
    S_(SEC_E_TARGET_UNKNOWN);
    S_(SEC_E_UNFINISHED_CONTEXT_DELETED);
    S_(SEC_E_UNSUPPORTED_FUNCTION);
    S_(SEC_E_UNTRUSTED_ROOT);
    S_(SEC_E_WRONG_PRINCIPAL);
    S_(SEC_I_COMPLETE_AND_CONTINUE);
    S_(SEC_I_COMPLETE_NEEDED);
    S_(SEC_I_CONTEXT_EXPIRED);
    S_(SEC_I_CONTINUE_NEEDED);
    S_(SEC_I_CONTINUE_NEEDED_MESSAGE_OK);
    S_(SEC_I_INCOMPLETE_CREDENTIALS);
    S_(SEC_I_MESSAGE_FRAGMENT);
    S_(SEC_I_RENEGOTIATE);
    default: oss << "Error<" << std::hex << status << ">";
  }
  #undef S_
  std::cout << oss.str() << '\n';
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

    ::SecBuffer &operator[] (size_t index) noexcept
    {
      return pBuffers[index];
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


void print_bufs (const char prefix[], buffer_t::list_t bufs) noexcept
{
#if WITH_LOGGING //{{{2
  std::cout << "    - " << prefix << ':';
  auto n = 0U;
  for (auto *it = bufs.pBuffers;  it != bufs.pBuffers + bufs.cBuffers;  ++it, ++n)
  {
    std::cout << ' ' << n << '=';
    switch (it->BufferType)
    {
      case SECBUFFER_STREAM_HEADER: std::cout << "header"; break;
      case SECBUFFER_STREAM_TRAILER: std::cout << "trailer"; break;
      case SECBUFFER_DATA: std::cout << "data"; break;
      case SECBUFFER_EMPTY: std::cout << "empty"; break;
      case SECBUFFER_ALERT: std::cout << "alert"; break;
      case SECBUFFER_TOKEN: std::cout << "token"; break;
      case SECBUFFER_EXTRA: std::cout << "extra"; break;
      case SECBUFFER_MISSING: std::cout << "missing"; break;
      case SECBUFFER_STREAM: std::cout << "stream"; break;
      default: std::cout << "XXX_" << it->BufferType; break;
    }
    std::cout << '<' << it->cbBuffer << '>';
  }
  std::cout << '\n';
#else
  (void)prefix;
  (void)bufs;
#endif //}}}2
}


inline void handle_out (pipe_t &pipe, buffer_t::list_t &out) noexcept
{
  auto &data = out[0];
  if (data.BufferType == SECBUFFER_TOKEN && data.cbBuffer > 0)
  {
    pipe.out_ptr += data.cbBuffer;
  }
}


size_t handle_extra (pipe_t &pipe, buffer_t::list_t &in, size_t index)
  noexcept
{
  auto &extra = in[index];
  if (extra.BufferType == SECBUFFER_EXTRA)
  {
    if (!pipe.incomplete_message.empty())
    {
      pipe.incomplete_message.erase(
        pipe.incomplete_message.begin(),
        pipe.incomplete_message.end() - extra.cbBuffer
      );
      return 0;
    }
    return extra.cbBuffer;
  }
  pipe.incomplete_message.clear();
  return 0;
}


bool handle_missing (pipe_t &pipe, buffer_t::list_t &in, std::error_code &error)
  noexcept
{
  auto &missing = in[1];
  if (missing.BufferType == SECBUFFER_MISSING && missing.cbBuffer > 0)
  {
    pipe.complete_message_size = missing.cbBuffer + pipe.incomplete_message.size();
    if (sal_unlikely(pipe.complete_message_size > pipe.max_message_size))
    {
      error = std::make_error_code(std::errc::no_buffer_space);
      return false;
    }
    pipe.incomplete_message.reserve(pipe.complete_message_size);
  }
  if (pipe.incomplete_message.empty())
  {
    // buffer first chunk, following chunks should be buffered by caller
    pipe.buffer_while_incomplete_message();
  }
  return true;
}


bool trusted_peer (pipe_t &pipe, std::error_code &error) noexcept
{
  if (pipe.factory->certificate_check)
  {
    certificate_t native_peer_certificate;
    auto status = ::QueryContextAttributes(&pipe.context,
      SECPKG_ATTR_REMOTE_CERT_CONTEXT,
      &native_peer_certificate.ref
    );
    if (status == SEC_E_OK)
    {
      return pipe.factory->certificate_check(
        sal::crypto::certificate_t::from_native_handle(
          std::move(native_peer_certificate)
        )
      );
    }
    else
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


void finish_handshake (pipe_t &pipe, std::error_code &error) noexcept
{
  if (trusted_peer(pipe, error))
  {
    LOG(std::cout << "    * connected");

    ::SecPkgContext_StreamSizes sizes{};
    auto status = ::QueryContextAttributes(&pipe.context,
      SECPKG_ATTR_STREAM_SIZES,
      &sizes
    );
    if (status == SEC_E_OK)
    {
      pipe.header_size = sizes.cbHeader;
      pipe.trailer_size = sizes.cbTrailer;
      pipe.max_message_size = sizes.cbMaximumMessage;
      pipe.handshake_result = std::make_error_code(std::errc::already_connected);
      error.clear();

      LOG(std::cout
        << ", header=" << pipe.header_size
        << ", trailer=" << pipe.trailer_size
        << ", max=" << pipe.max_message_size
        << '\n';
      );
    }
    else
    {
      LOG(std::cout << ", failed to get sizes\n");
      error.assign(status, category());
    }
  }
  else
  {
    LOG(std::cout << "    * not trusted\n");
    pipe.handshake_result = std::make_error_code(std::errc::connection_aborted);
    error = pipe.handshake_result;
  }
}


} // namespace


void pipe_t::ctor (std::error_code &error) noexcept
{
  if (factory->server)
  {
    side = 'S';

    if (factory->datagram)
    {
      context_request |= ASC_REQ_DATAGRAM;
    }
    else
    {
      context_request |= ASC_REQ_STREAM;
    }
    if (factory->mutual_auth)
    {
      context_request |= ASC_REQ_MUTUAL_AUTH;
    }
  }
  else
  {
    side = 'C';

    if (factory->datagram)
    {
      context_request |= ISC_REQ_DATAGRAM;
    }
    else
    {
      context_request |= ISC_REQ_STREAM;
    }
    if (factory->mutual_auth)
    {
      context_request |= ISC_REQ_MUTUAL_AUTH;
    }
  }

  error.clear();
}


pipe_t::~pipe_t () noexcept
{
  (void)::DeleteSecurityContext(&context);
}


bool pipe_t::buffer_while_incomplete_message ()
{
  //LOG(std::cout << "    - buffering " << incomplete_message.size());
  incomplete_message.insert(incomplete_message.end(), in_first, in_last);
  in_first = in_ptr = incomplete_message.data();
  in_last = in_first + incomplete_message.size();
  //LOG(std::cout << " -> " << incomplete_message.size());

  if (incomplete_message.size() < complete_message_size)
  {
    //LOG(std::cout << ", not ready\n");
    return true;
  }

  //LOG(std::cout << ", ready\n");
  complete_message_size = 0;
  return false;
}


std::pair<size_t, size_t> pipe_t::handshake (std::error_code &error)
{
  size_t consumed = in_last - in_first, not_consumed = 0;
  if (!incomplete_message.empty() && buffer_while_incomplete_message())
  {
    return {consumed, 0};
  }

  LOG(std::cout << side
    << "> handshake, IN: " << (in_last - in_ptr)
    << ", ROOM: " << (out_last - out_ptr) << '\n');

  SECURITY_STATUS status;
  do
  {
    buffer_t in_[] =
    {
      token_t{in_ptr, size_t(in_last - in_ptr)},
      empty_t{},
      extra_t{},
    };

    uint8_t alert[64];
    buffer_t out_[] =
    {
      token_t{out_ptr, size_t(out_last - out_ptr)},
      alert_t{alert, sizeof(alert)},
    };
    buffer_t::list_t in(in_), out(out_);

    if (factory->server)
    {
      status = call(::AcceptSecurityContext,
        &factory->credentials,              // phCredentials
        (is_valid() ? &context : nullptr),  // phContext
        &in,                                // pInput
        context_request,                    // fContextReq
        0,                                  // TargetDataRep
        &context,                           // phNewContext
        &out,                               // pOutput
        &context_flags,                     // pfContextAttr
        nullptr                             // ptsTimeStamp
      );
    }
    else
    {
      status = call(::InitializeSecurityContext,
        &factory->credentials,              // phCredentials
        (is_valid() ? &context : nullptr),  // phContext
        (SEC_CHAR *)peer_name.c_str(),      // pszTargetName
        context_request,                    // fContextReq
        0,                                  // Reserved
        0,                                  // TargetDataRep
        (is_valid() ? &in : nullptr),       // pInput
        0,                                  // Reserved2
        &context,                           // phNewContext
        &out,                               // pOutput
        &context_flags,                     // pfContextAttr
        nullptr                             // ptsExpiry
      );
    }

#if WITH_LOGGING
    print_bufs("In", in);
    print_bufs("Out", out);
#endif

    switch (status)
    {
      case SEC_E_OK:
        finish_handshake(*this, error);
        [[fallthrough]];

      case SEC_I_CONTINUE_NEEDED:
      case SEC_I_MESSAGE_FRAGMENT:
        handle_out(*this, out);
        not_consumed = handle_extra(*this, in, 1);
        break;

      case SEC_E_BUFFER_TOO_SMALL:
      case SEC_E_INSUFFICIENT_MEMORY:
        error = std::make_error_code(std::errc::no_buffer_space);
        return {};

      case SEC_E_INCOMPLETE_MESSAGE:
        if (!handle_missing(*this, in, error))
        {
          handshake_result = error;
        }
        break;

      default:
        handshake_result.assign(status, category());
        error = handshake_result;
        return {};
    }
  } while (status == SEC_I_MESSAGE_FRAGMENT);

  LOG(std::cout
    << "    > IN: " << (in_last - in_ptr)
    << ", OUT: " << (out_ptr - out_first) << '\n');

  return {consumed - not_consumed, out_ptr - out_first};
}


std::pair<size_t, size_t> pipe_t::encrypt (std::error_code &error)
{
  LOG(std::cout << side
    << "> encrypt, IN " << (in_last - in_ptr)
    << ", ROOM " << (out_last - out_ptr) << '\n');

  const size_t message_size = in_last - in_first, room = out_last - out_ptr;
  if (message_size > max_message_size)
  {
    error = std::make_error_code(std::errc::message_size);
    return {};
  }
  if (header_size + message_size + trailer_size > room)
  {
    error = std::make_error_code(std::errc::no_buffer_space);
    return {};
  }

  buffer_t io_[] =
  {
    header_t{out_ptr, header_size},
    data_t{out_ptr += header_size, message_size},
    trailer_t{out_ptr += message_size, trailer_size},
    empty_t{},
  };
  buffer_t::list_t io(io_);

  std::uninitialized_copy_n(in_first, message_size,
    sal::__bits::make_output_iterator(out_first + header_size, out_last)
  );

  auto status = call(::EncryptMessage, &context, 0, &io, 0);
  if (status == SEC_E_OK)
  {
    out_ptr = out_first + io[0].cbBuffer + io[1].cbBuffer + io[2].cbBuffer;
    error.clear();
  }
  else
  {
    error.assign(status, category());
  }

  print_bufs("IO", io);
  LOG(std::cout
    << "    > IN: " << (in_last - in_ptr)
    << ", OUT: " << (out_ptr - out_first) << '\n');

  return {message_size, out_ptr - out_first};
}


std::pair<size_t, size_t> pipe_t::decrypt (std::error_code &error)
{
  size_t consumed = in_last - in_first, not_consumed = 0, room = out_last - out_ptr;
  if (!incomplete_message.empty() && buffer_while_incomplete_message())
  {
    return {consumed, 0};
  }

  LOG(std::cout << side
    << "> decrypt, IN " << (in_last - in_ptr)
    << ", ROOM " << (out_last - out_ptr) << '\n');

  buffer_t io_[] =
  {
    data_t{in_ptr, size_t(in_last - in_ptr)},
    empty_t{},
    empty_t{},
    empty_t{},
  };
  buffer_t::list_t io(io_);

  switch (auto status = call(::DecryptMessage, &context, &io, 0, nullptr))
  {
    case SEC_E_OK:
      if (io[1].cbBuffer <= room)
      {
        // TODO we can do in-place decrypt (consider after OpenSSL impl)
        std::uninitialized_copy_n(
          static_cast<const uint8_t *>(io[1].pvBuffer),
          io[1].cbBuffer,
          sal::__bits::make_output_iterator(out_ptr, out_last)
        );
        out_ptr += io[1].cbBuffer;
        not_consumed = handle_extra(*this, io, 3);
        error.clear();
      }
      else
      {
        // can do output buffering, but don't want to, it is application
        // responsibility to provide sufficiently sized buffer
        error = std::make_error_code(std::errc::no_buffer_space);
      }
      break;

    case SEC_E_INCOMPLETE_MESSAGE:
      handle_missing(*this, io, error);
      break;

    default:
      error.assign(status, category());
      break;
  }

  print_bufs("IO", io);
  LOG(std::cout
    << "    > IN: " << (in_last - in_ptr)
    << ", OUT: " << (out_ptr - out_first) << '\n');

  return {consumed - not_consumed, out_ptr - out_first};
}


void pipe_factory_t::ctor (std::error_code &error) noexcept
{
  SCHANNEL_CRED auth_data{};
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


pipe_factory_t::~pipe_factory_t () noexcept
{
  (void)::FreeCredentialsHandle(&credentials);
}


#endif // }}}1


} // namespace crypto::__bits


__sal_end
