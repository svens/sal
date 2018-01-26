#include <sal/crypto/__bits/pipe.hpp>

#if __sal_os_macos
  #include <Security/SecIdentity.h>
#endif


__sal_begin


namespace crypto::__bits {


#if __sal_os_linux //{{{1


#elif __sal_os_macos //{{{1


#define LOG(x) //x


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
  if (pipe.in_first < pipe.in_last)
  {
    OSStatus result = errSecSuccess;
    size_t have = pipe.in_last - pipe.in_first;
    if (have < *size)
    {
      *size = have;
      result = errSSLWouldBlock;
      LOG(std::cout << ", block: " << have << '\n');
    }
    else
    {
      LOG(std::cout << ", ok\n");
    }
    std::uninitialized_copy_n(
      pipe.in_first, *size, static_cast<uint8_t *>(data)
    );
    pipe.in_first += *size;
    return result;
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
    OSStatus result = errSecSuccess;
    size_t room = pipe.out_last - pipe.out_ptr;
    if (*size > room)
    {
      *size = room;
      result = errSSLWouldBlock;
      LOG(std::cout << ", block: " << room << '\n');
    }
    else
    {
      LOG(std::cout << ", ok\n");
    }
    pipe.out_ptr = std::uninitialized_copy_n(
      static_cast<const uint8_t *>(data), *size, pipe.out_ptr
    );
    return result;
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
    auto break_on_auth = pipe.factory->inbound
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


bool is_trusted_peer_certificate (const pipe_t &pipe, std::error_code &error)
  noexcept
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
    factory->inbound ? kSSLServerSide : kSSLClientSide,
    stream_oriented ? kSSLStreamType : kSSLDatagramType
  );
  if (!context)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return;
  }

  side = factory->inbound ? 'S' : 'C';
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


void pipe_t::handshake (std::error_code &error) noexcept
{
  if (handshake_result)
  {
    error = handshake_result;
    return;
  }

  LOG(std::cout << side
    << "> handshake, IN: " << (in_last - in_first)
    << ", OUT: " << (out_last - out_ptr) << '\n');

  auto status = ::SSLHandshake(context.ref);

  LOG(std::cout
    << "    < IN: " << (in_last - in_first)
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
    if (is_trusted_peer_certificate(*this, error))
    {
      handshake(error);
    }
    else
    {
      handshake_result = std::make_error_code(std::errc::connection_aborted);
    }
  }
  else
  {
    handshake_result = std::make_error_code(std::errc::connection_aborted);
    error.assign(status, category());
    LOG(std::cout << side << "> error\n");
  }
}


void pipe_t::encrypt (std::error_code &error) noexcept
{
  if (handshake_result != std::errc::already_connected)
  {
    error = std::make_error_code(std::errc::not_connected);
    return;
  }

  LOG(std::cout << side
    << "> encrypt, IN: " << (in_last - in_first)
    << ", OUT: " << (out_last - out_ptr) << '\n');

  size_t processed = 0U;
  auto status = ::SSLWrite(context.ref,
    in_first,
    (in_last - in_first),
    &processed
  );
  in_first += processed;

  LOG(std::cout
    << "    < IN: " << (in_last - in_first)
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
}


void pipe_t::decrypt (std::error_code &error) noexcept
{
  if (handshake_result != std::errc::already_connected)
  {
    error = std::make_error_code(std::errc::not_connected);
    return;
  }

  LOG(std::cout << side
    << "> decrypt, IN: " << (in_last - in_first)
    << ", OUT: " << (out_last - out_ptr) << '\n');

  size_t processed = 0U;
  auto status = ::SSLRead(context.ref,
    out_ptr,
    (out_last - out_ptr),
    &processed
  );
  out_ptr += processed;

  LOG(std::cout
    << "    < IN: " << (in_last - in_first)
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
  else
  {
    error.assign(status, category());
    LOG(std::cout << side << "> error\n");
  }
}


void pipe_factory_t::ctor (std::error_code &error) noexcept
{
  error.clear();
}


#elif __sal_os_windows //{{{1


#endif // }}}1


} // namespace crypto::__bits


__sal_end
