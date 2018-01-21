#include <sal/crypto/__bits/pipe.hpp>

#if __sal_os_macos
  #include <Security/SecIdentity.h>
#endif


#include <iostream>


__sal_begin


namespace crypto::__bits {


#if __sal_os_linux //{{{1


#elif __sal_os_macos //{{{1


namespace {


inline pipe_t &to_pipe (::SSLConnectionRef connection) noexcept
{
  return *const_cast<pipe_t *>(static_cast<const pipe_t *>(connection));
}


OSStatus pipe_read (::SSLConnectionRef connection, void *data, size_t *size)
  noexcept
{
  auto &pipe = to_pipe(connection);
  if (pipe.recv_first < pipe.recv_last)
  {
    size_t have = pipe.recv_last - pipe.recv_first;
    if (*size > have)
    {
      *size = have;
    }
    std::uninitialized_copy_n(
      pipe.recv_first, *size, static_cast<uint8_t *>(data)
    );
    pipe.recv_first += *size;
    return errSecSuccess;
  }
  *size = 0;
  return errSSLWouldBlock;
}


OSStatus pipe_write (::SSLConnectionRef connection, const void *data, size_t *size)
  noexcept
{
  auto &pipe = to_pipe(connection);
  if (pipe.send_ptr < pipe.send_last)
  {
    size_t room = pipe.send_last - pipe.send_ptr;
    if (*size > room)
    {
      *size = room;
    }
    pipe.send_ptr = std::uninitialized_copy_n(
      static_cast<const uint8_t *>(data), *size, pipe.send_ptr
    );
    return errSecSuccess;
  }
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

  if (set_io(*this, error)
    && set_connection(*this, error)
    && set_peer_name(*this, error)
    && set_mutual_auth(*this, error)
    && set_certificate(*this, error)
    && set_certificate_check(*this, error))
  {
    state = factory->inbound
      ? &pipe_t::server_handshake
      : &pipe_t::client_handshake;
    error.clear();
  }
}


void pipe_t::client_handshake (std::error_code &error) noexcept
{
  auto status = ::SSLHandshake(context.ref);
  if (status == errSecSuccess)
  {
    state = &pipe_t::connected;
    error.clear();
  }
  else if (status == errSSLWouldBlock)
  {
    error.clear();
  }
  else if (status == errSSLPeerAuthCompleted)
  {
    if (is_trusted_peer_certificate(*this, error))
    {
      error.clear();
    }
    else
    {
      state = {};
      error.assign(errSSLPeerCertUnknown, category());
    }
  }
  else
  {
    state = {};
    error.assign(status, category());
  }
}


void pipe_t::server_handshake (std::error_code &error) noexcept
{
  // for Apple's SecureTransport, same SSLHandshake
  client_handshake(error);
}


void pipe_t::connected (std::error_code &error) noexcept
{
  std::cout << "running(" << std::string(recv_first, recv_last) << ")\n";
  error.clear();
}


void pipe_factory_t::ctor (std::error_code &error) noexcept
{
  error.clear();
}


#elif __sal_os_windows //{{{1


#endif // }}}1


} // namespace crypto::__bits


__sal_end
