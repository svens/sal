#include <sal/crypto/channel.hpp>

#if __sal_os_linux //{{{1
  // TODO
#elif __sal_os_macos //{{{1
  #include <Security/SecIdentity.h>
#elif __sal_os_windows //{{{1
  // TODO
#endif //}}}1


#define WITH_LOGGING 0
#if WITH_LOGGING
  #include <iostream>
  #include <sstream>
  #define LOG(x) x
#else
  #define LOG(x)
#endif


__sal_begin


namespace crypto {


#if __sal_os_linux //{{{1


void channel_context_t::ctor (std::error_code &error) noexcept
{
  error.clear();
}


void channel_t::ctor (std::error_code &error) noexcept
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


struct crypto_syscall
{
  const uint8_t *in_first;
  const uint8_t *in_last;
  const uint8_t *in_ptr;

  channel_t::buffer_manager_t &buffer_manager;

  uintptr_t user_data{};
  uint8_t *out_first{};
  uint8_t *out_last{};
  uint8_t *out_ptr{};


  crypto_syscall (const uint8_t *first,
      const uint8_t *last,
      channel_t::buffer_manager_t &buffer_manager) noexcept
    : in_first(first)
    , in_last(last)
    , in_ptr(first)
    , buffer_manager(buffer_manager)
  { }


  ~crypto_syscall () noexcept
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

  crypto_syscall (const crypto_syscall &) = delete;
  crypto_syscall &operator= (const crypto_syscall &) = delete;
};


void crypto_syscall::drain_system_buffer (__bits::channel_t &impl,
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
  auto &call = *static_cast<crypto_syscall *>(channel.syscall);

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
  auto &call = *static_cast<crypto_syscall *>(channel.syscall);
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
  if (impl.context->mutual_auth)
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
  if (impl.context->certificate)
  {
    unique_ref<::SecIdentityRef> identity;
    auto status = ::SecIdentityCreateWithCertificate(nullptr,
      impl.context->certificate.native_handle().ref,
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
  if (impl.context->certificate_check)
  {
    auto break_on_auth = impl.server
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

  return impl.context->certificate_check(peer_certificate);
}


} // namespace


void channel_context_t::ctor (std::error_code &error) noexcept
{
  error.clear();
}


void channel_t::ctor (std::error_code &error) noexcept
{
  impl_->handle.ref = ::SSLCreateContext(nullptr,
    impl_->server ? ::kSSLServerSide : ::kSSLClientSide,
    impl_->context->datagram ? ::kSSLDatagramType : ::kSSLStreamType
  );
  if (!impl_->handle)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }

  if (set_io(*impl_, error)
    && set_connection(*impl_, error)
    && set_peer_name(*impl_, error)
    && set_mutual_auth(*impl_, error)
    && set_certificate(*impl_, error)
    && set_certificate_check(*impl_, error))
  {
    error.clear();
  }
}


size_t channel_t::handshake (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  auto &impl = *impl_;

  LOG(std::cout << (impl.server ? "server" : "client")
    << "> handshake: " << size << '\n'
  );

  crypto_syscall syscall(data, data + size, buffer_manager);
  for (;;)
  {
    switch (auto status = syscall.handshake(impl))
    {
      case ::errSecSuccess:
        LOG(std::cout << "    | connected (" << (syscall.in_ptr - syscall.in_first) << ")\n");
        impl.status = std::make_error_code(std::errc::already_connected);
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
          impl.status = std::make_error_code(std::errc::connection_aborted);
          break;
        }
        continue;

      default:
        LOG(std::cout << "    | error\n");
        impl.status = std::make_error_code(std::errc::connection_aborted);
        error.assign(status, category());
        break;
    }

    return syscall.in_ptr - syscall.in_first;
  }
}


void channel_t::encrypt (const uint8_t *data, size_t size,
  buffer_manager_t &buffer_manager,
  std::error_code &error) noexcept
{
  LOG(std::cout << (impl_->server ? "server" : "client")
    << "> encrypt: " << size << '\n'
  );

  size_t processed{};
  crypto_syscall syscall(data, data + size, buffer_manager);
  switch (auto status = syscall.encrypt(*impl_, &processed))
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
  LOG(std::cout << (impl_->server ? "server" : "client")
    << "> decrypt: " << size << '\n'
  );

  size_t processed{};
  crypto_syscall syscall(data, data + size, buffer_manager);
  switch (auto status = syscall.decrypt(*impl_, &processed))
  {
    case ::errSecSuccess:
      syscall.drain_system_buffer(*impl_, &processed);
      LOG(std::cout << "    | ready " << processed
        << ", used " << (syscall.in_ptr - syscall.in_first)
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

  return syscall.in_ptr - syscall.in_first;
}


#elif __sal_os_windows //{{{1


void channel_context_t::ctor (std::error_code &error) noexcept
{
  error.clear();
}


void channel_t::ctor (std::error_code &error) noexcept
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


#endif //}}}1


} // namespace crypto


__sal_end
