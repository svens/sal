#include <sal/crypto/channel.hpp>

#if __sal_os_linux //{{{1
  // TODO
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


void __bits::channel_factory_t::ctor (std::error_code &error) noexcept
{
  error.clear();
}


void __bits::channel_t::ctor (std::error_code &error) noexcept
{
  error.clear();
}


size_t channel_t::handshake (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  (void)data;
  (void)size;
  (void)buffer_manager;
  error.clear();
  return {};
}


void channel_t::encrypt (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  (void)data;
  (void)size;
  (void)buffer_manager;
  error.clear();
}


size_t channel_t::decrypt (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  (void)data;
  (void)size;
  (void)buffer_manager;
  error.clear();
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

  uintptr_t user_data{};
  uint8_t *out_first{};
  uint8_t *out_last{};
  uint8_t *out_ptr{};


  crypto_call_t (const uint8_t *first,
      const uint8_t *last,
      channel_t::buffer_manager_t &buffer_manager) noexcept
    : in_first(first)
    , in_last(last)
    , in_ptr(first)
    , buffer_manager(buffer_manager)
  { }


  ~crypto_call_t () noexcept
  {
    flush_buffer();
  }


  ::OSStatus handshake (__bits::channel_t &impl) noexcept
  {
    impl.syscall = this;
    return ::SSLHandshake(impl.handle.ref);
  }


  ::OSStatus encrypt (__bits::channel_t &impl, size_t *processed) noexcept
  {
    impl.syscall = this;
    return ::SSLWrite(impl.handle.ref, in_ptr, in_last - in_ptr, processed);
  }


  ::OSStatus decrypt (__bits::channel_t &impl, size_t *processed) noexcept
  {
    impl.syscall = this;
    if (auto buffer_size = ensure_available_buffer(in_last - in_ptr))
    {
      auto status = ::SSLRead(impl.handle.ref, out_ptr, buffer_size, processed);
      out_ptr += *processed;
      return status;
    }
    return ::errSSLBufferOverflow;
  }


  size_t ensure_available_buffer (size_t requested_size) noexcept
  {
    if (out_ptr + requested_size > out_last)
    {
      flush_buffer();
      auto size = requested_size;
      user_data = buffer_manager.alloc(&out_ptr, &size);
      if (out_ptr && size)
      {
        out_first = out_ptr;
        out_last = out_ptr + size;
      }
      else
      {
        out_ptr = out_first = out_last = nullptr;
      }
    }
    return out_last - out_ptr;
  }


  void flush_buffer () noexcept
  {
    buffer_manager.ready(user_data, out_first, out_ptr - out_first);
  }

  void drain_system_buffer (__bits::channel_t &impl, size_t *processed)
    noexcept;

  crypto_call_t (const crypto_call_t &) = delete;
  crypto_call_t &operator= (const crypto_call_t &) = delete;
};


void crypto_call_t::drain_system_buffer (__bits::channel_t &impl,
  size_t *processed) noexcept
{
  size_t system_buffer_size{};
  ::SSLGetBufferedReadSize(impl.handle.ref, &system_buffer_size);

  while (system_buffer_size)
  {
    if (auto buffer_size = ensure_available_buffer(system_buffer_size))
    {
      size_t read;
      auto status = ::SSLRead(impl.handle.ref, out_ptr, buffer_size, &read);
      if (status == ::errSecSuccess)
      {
        out_ptr += read;
        *processed += read;
        system_buffer_size -= read;
        continue;
      }
    }
    return;
  }
}


inline __bits::channel_t &to_channel (::SSLConnectionRef connection) noexcept
{
  return *const_cast<__bits::channel_t *>(
    static_cast<const __bits::channel_t *>(connection)
  );
}


::OSStatus channel_read (::SSLConnectionRef connection,
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


::OSStatus channel_write (::SSLConnectionRef connection,
  const void *data, size_t *size) noexcept
{
  LOG(std::cout << "    | write " << *size);

  auto &channel = to_channel(connection);
  auto &call = *static_cast<crypto_call_t *>(channel.syscall);
  auto data_ptr = static_cast<const uint8_t *>(data);
  auto data_size = *size;
  *size = 0;

  while (auto buffer_size = call.ensure_available_buffer(data_size))
  {
    if (data_size <= buffer_size)
    {
      LOG(std::cout << ", all\n");
      call.out_ptr = std::uninitialized_copy_n(data_ptr, data_size, call.out_ptr);
      *size += data_size;
      return ::errSecSuccess;
    }

    LOG(std::cout << '[' << buffer_size << ']');
    call.out_ptr = std::uninitialized_copy_n(data_ptr, buffer_size, call.out_ptr);
    data_ptr += buffer_size;
    data_size -= buffer_size;
    *size += buffer_size;
  }

  LOG(std::cout << ", no buf\n");
  return ::errSSLBufferOverflow;
}


bool set_io (__bits::channel_t &impl, std::error_code &error) noexcept
{
  auto status = ::SSLSetIOFuncs(impl.handle.ref,
    &channel_read,
    &channel_write
  );
  if (status == ::errSecSuccess)
  {
    return true;
  }
  error.assign(status, category());
  return false;
}


bool set_connection (__bits::channel_t &impl, std::error_code &error) noexcept
{
  auto status = ::SSLSetConnection(impl.handle.ref, &impl);
  if (status == ::errSecSuccess)
  {
    return true;
  }
  error.assign(status, category());
  return false;
}


bool set_peer_name (__bits::channel_t &impl, std::error_code &error) noexcept
{
  if (!impl.peer_name.empty())
  {
    auto status = ::SSLSetPeerDomainName(impl.handle.ref,
      impl.peer_name.data(),
      impl.peer_name.size()
    );
    if (status != ::errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool set_mutual_auth (__bits::channel_t &impl, std::error_code &error) noexcept
{
  if (impl.mutual_auth)
  {
    auto status = ::SSLSetClientSideAuthenticate(impl.handle.ref,
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


bool set_certificate (__bits::channel_t &impl, std::error_code &error) noexcept
{
  if (impl.factory->certificate)
  {
    unique_ref<::SecIdentityRef> identity;
    auto status = ::SecIdentityCreateWithCertificate(nullptr,
      impl.factory->certificate.ref,
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

    status = ::SSLSetCertificate(impl.handle.ref, certificates.ref);
    if (status != ::errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool set_certificate_check (__bits::channel_t &impl, std::error_code &error)
  noexcept
{
  if (impl.factory->certificate_check)
  {
    auto break_on_auth = impl.factory->server
      ? ::kSSLSessionOptionBreakOnClientAuth
      : ::kSSLSessionOptionBreakOnServerAuth
    ;
    auto status = ::SSLSetSessionOption(impl.handle.ref, break_on_auth, true);
    if (status != ::errSecSuccess)
    {
      error.assign(status, category());
      return false;
    }
  }
  return true;
}


bool is_trusted_peer (__bits::channel_t &impl, std::error_code &error) noexcept
{
  unique_ref<::SecTrustRef> trust{};
  auto status = ::SSLCopyPeerTrust(impl.handle.ref, &trust.ref);
  if (status != ::errSecSuccess)
  {
    error.assign(status, category());
    return false;
  }

  auto peer_certificate = crypto::certificate_t::from_native_handle(
    ::SecTrustGetCertificateAtIndex(trust.ref, 0)
  );
  ::CFRetain(peer_certificate.native_handle().ref);

  return impl.factory->certificate_check(peer_certificate);
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
  auto &impl = *impl_;

  LOG(std::cout << (impl.factory->server ? "server" : "client")
    << "> handshake: " << size << '\n'
  );

  crypto_call_t call(data, data + size, buffer_manager);
  for (;;)
  {
    switch (auto status = call.handshake(impl))
    {
      case ::errSecSuccess:
        LOG(std::cout << "    | connected (" << (call.in_ptr - call.in_first) << ")\n");
        impl.handshake_status = std::make_error_code(std::errc::already_connected);
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
        if (!is_trusted_peer(impl, error))
        {
          impl.handshake_status = std::make_error_code(std::errc::connection_aborted);
          break;
        }
        continue;

      default:
        LOG(std::cout << "    | error\n");
        impl.handshake_status = std::make_error_code(std::errc::connection_aborted);
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
      call.drain_system_buffer(*impl_, &processed);
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
    context_request = ASC_REQ_ALLOCATE_MEMORY;
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
    context_request = ISC_REQ_ALLOCATE_MEMORY;
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
  size_t *size)
{
  // add new data to buffer
  /*
  LOG(std::cout << "    | buffer: " << channel.in.size()
    << " + " << *size
  );
  */
  channel.in.insert(channel.in.end(), *data, *data + *size);
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


bool flush_out (__bits::channel_t &channel,
  channel_t::buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  LOG(std::cout << "    | flush " << channel.out_size);
  while (channel.out_size)
  {
    uint8_t *out_ptr;
    size_t out_size;
    auto user_data = buffer_manager.alloc(&out_ptr, &out_size);
    if (out_ptr && out_size)
    {
      if (out_size > channel.out_size)
      {
        out_size = channel.out_size;
      }
      std::uninitialized_copy_n(channel.out_ptr, out_size,
        sal::__bits::make_output_iterator(out_ptr, out_ptr + out_size)
      );
      buffer_manager.ready(user_data, out_ptr, out_size);
      channel.out_size -= out_size;
      channel.out_ptr += out_size;
    }
    else
    {
      LOG(std::cout << ", remaining " << channel.out_size << '\n');
      error = std::make_error_code(std::errc::no_buffer_space);
      return false;
    }
  }
  LOG(std::cout << ", done\n");
  channel.out.reset();
  return true;
}


bool handle_out (__bits::channel_t &channel,
  buffer_t::list_t &out,
  channel_t::buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  auto &data = out[0];
  if (data.BufferType == SECBUFFER_TOKEN && data.pvBuffer && data.cbBuffer)
  {
    channel.out.reset(data.pvBuffer);
    channel.out_size = data.cbBuffer;
    channel.out_ptr = static_cast<const uint8_t *>(channel.out.get());
    return flush_out(channel, buffer_manager, error);
  }
  return true;
}


size_t handle_extra (__bits::channel_t &channel,
  buffer_t::list_t &in,
  size_t index) noexcept
{
  auto &extra = in[index];
  if (extra.BufferType == SECBUFFER_EXTRA)
  {
    if (!channel.in.empty())
    {
      channel.in.erase(channel.in.begin(), channel.in.end() - extra.cbBuffer);
      return 0;
    }
    return extra.cbBuffer;
  }
  channel.in.clear();
  return 0;
}


bool handle_missing (__bits::channel_t &channel,
  buffer_t::list_t &in,
  const uint8_t *data,
  size_t data_size,
  std::error_code &error) noexcept
{
  auto &missing = in[1];
  if (missing.BufferType == SECBUFFER_MISSING && missing.cbBuffer > 0)
  {
    channel.complete_message_size = missing.cbBuffer + channel.in.size();
    if (channel.complete_message_size > channel.max_message_size)
    {
      error = std::make_error_code(std::errc::no_buffer_space);
      return false;
    }
    channel.in.reserve(channel.complete_message_size);
  }
  if (channel.in.empty())
  {
    try
    {
      buffer_while_incomplete_message(channel, &data, &data_size);
    }
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
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
      channel.handshake_status = std::make_error_code(
        std::errc::already_connected
      );
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
      channel.handshake_status = std::make_error_code(
        std::errc::connection_aborted
      );
    }
  }
  else
  {
    LOG(std::cout << ", not trusted\n");
    error = std::make_error_code(std::errc::connection_aborted);
    channel.handshake_status = error;
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

  // do nothing until output buffer flushed
  if (channel.out)
  {
    if (!flush_out(channel, buffer_manager, error) || !data || !size)
    {
      return {};
    }
  }

  // if we know expected message size, just keep buffering until ready
  size_t consumed = size, not_consumed = 0U;
  try
  {
    if (!channel.in.empty()
      && buffer_while_incomplete_message(channel, &data, &size))
    {
      return consumed;
    }
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return {};
  }

  SECURITY_STATUS status;
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
      token_t{},
      alert_t{},
    };
    buffer_t::list_t in(in_), out(out_);

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
        if (handle_out(channel, out, buffer_manager, error))
        {
          not_consumed = handle_extra(channel, in, 1);
        }
        break;

      case SEC_E_INCOMPLETE_MESSAGE:
        if (!handle_missing(channel, in, data, size, error))
        {
          channel.handshake_status = std::make_error_code(
            std::errc::connection_aborted
          );
        }
        break;

      default:
        channel.handshake_status = std::make_error_code(
          std::errc::connection_aborted
        );
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
  (void)data;
  (void)size;
  (void)buffer_manager;
  error.clear();
}


size_t channel_t::decrypt (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  (void)data;
  (void)size;
  (void)buffer_manager;
  error.clear();
  return {};
}


#endif //}}}1


} // namespace crypto


__sal_end
