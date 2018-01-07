#include <sal/crypto/__bits/pipe.hpp>
#include <sal/crypto/certificate.hpp>
#include <sal/crypto/error.hpp>

#if __sal_os_macos //{{{1
#elif __sal_os_linux //{{{1
#elif __sal_os_windows //{{{1
  #include <initializer_list>
  #include <schannel.h>
  #include <security.h>
  #pragma comment(lib, "secur32")
#endif //}}}1


#define WITH_LOGGING 1
#if defined(WITH_LOGGING)
  #include <iostream>
  #include <sstream>
#endif


__sal_begin


namespace crypto::__bits {


#if __sal_os_macos || __sal_os_linux //{{{1


void pipe_factory_t::ctor (std::error_code &error) noexcept
{
  error.clear();
}


pipe_factory_t::~pipe_factory_t () noexcept
{
}


pipe_t::~pipe_t () noexcept
{
}


void pipe_t::client_handshake (std::error_code &error) noexcept
{
  error.clear();
}


void pipe_t::server_handshake (std::error_code &error) noexcept
{
  error.clear();
}


void pipe_t::exchange_messages (std::error_code &error) noexcept
{
  error.clear();
}


#elif __sal_os_windows //{{{1


void pipe_factory_t::ctor (std::error_code &error) noexcept
{
  SCHANNEL_CRED auth_data{};
  auth_data.dwVersion = SCHANNEL_CRED_VERSION;

  auth_data.dwFlags = SCH_CRED_NO_DEFAULT_CREDS;
  if (manual_certificate_check_)
  {
    auth_data.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;
  }

  if (!certificate_.is_null())
  {
    auth_data.paCred = &certificate_.ref;
    auth_data.cCreds = 1;
  }

  auto result = ::AcquireCredentialsHandle(
    nullptr,
    UNISP_NAME,
    (inbound_ ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND),
    nullptr,
    &auth_data,
    nullptr,
    nullptr,
    &credentials_,
    nullptr
  );

  if (result == SEC_E_OK)
  {
    error.clear();
  }
  else
  {
    error.assign(result, category());
  }
}


pipe_factory_t::~pipe_factory_t () noexcept
{
  (void)::FreeCredentialsHandle(&credentials_);
}


namespace {


inline auto handle_result (SECURITY_STATUS status, const char *func, char side)
{
#if WITH_LOGGING //{{{2
  std::ostringstream oss;
  oss << side << "> " << func << ": ";
  #define S_(s) case s: oss << #s; break
  switch (status)
  {
    S_(SEC_E_OK);
    S_(SEC_E_ALGORITHM_MISMATCH);
    S_(SEC_E_APPLICATION_PROTOCOL_MISMATCH);
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
#endif //}}}2
  return status;
}

#define call(func, ...) handle_result(func(__VA_ARGS__), #func, side_)


struct buffer_t
  : public ::SecBuffer
{
  buffer_t (int type, uint8_t *first, uint8_t *last) noexcept
  {
    BufferType = type;
    pvBuffer = first;
    cbBuffer = static_cast<ULONG>(last - first);
  }

  struct list_t
    : public ::SecBufferDesc
  {
    template <ULONG N>
    list_t (buffer_t (&&bufs)[N]) noexcept
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
struct basic_buffer_t: buffer_t
{
  basic_buffer_t (uint8_t *first = nullptr, uint8_t *last = nullptr) noexcept
    : buffer_t(Type, first, last)
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
  std::cout << "\tFlags:\n";
  #define F_(f) if ((flags & ISC_REQ_##f) == ISC_REQ_##f) std::cout << "\t* " #f "\n"
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
#endif //}}}2
  (void)flags;
}


void print_bufs (buffer_t::list_t bufs) noexcept
{
#if WITH_LOGGING //{{{2
  auto n = 0;
  for (auto *it = bufs.pBuffers;  it != bufs.pBuffers + bufs.cBuffers;  ++it)
  {
    n++;
    if (it->BufferType == SECBUFFER_STREAM_HEADER)
    {
      std::cout << '\t' << n << ") header (" << it->cbBuffer << ")\n";
    }
    else if (it->BufferType == SECBUFFER_STREAM_TRAILER)
    {
      std::cout << '\t' << n << ") trailer (" << it->cbBuffer << ")\n";
    }
    else if (it->BufferType == SECBUFFER_DATA)
    {
      std::cout << '\t' << n << ") data (" << it->cbBuffer << ")\n";
    }
    else if (it->BufferType == SECBUFFER_EMPTY)
    {
      std::cout << '\t' << n << ") empty (" << it->cbBuffer << ")\n";
    }
    else if (it->BufferType == SECBUFFER_ALERT)
    {
      std::cout << '\t' << n << ") alert (" << it->cbBuffer << ")\n";
    }
    else if (it->BufferType == SECBUFFER_TOKEN)
    {
      std::cout << '\t' << n << ") token (" << it->cbBuffer << ")\n";
    }
    else if (it->BufferType == SECBUFFER_EXTRA)
    {
      std::cout << '\t' << n << ") extra (" << it->cbBuffer << ")\n";
    }
    else if (it->BufferType == SECBUFFER_MISSING)
    {
      std::cout << '\t' << n << ") missing (" << it->cbBuffer << ")\n";
    }
    else if (it->BufferType == SECBUFFER_STREAM)
    {
      std::cout << '\t' << n << ") stream (" << it->cbBuffer << ")\n";
    }
    else
    {
      std::cout << '\t' << n << ") [" << it->BufferType << "] (" << it->cbBuffer << ")\n";
    }
  }
#endif //}}}2
  (void)bufs;
}


bool finish_handshake (pipe_t &pipe, std::error_code &error) noexcept
{
  ::SecPkgContext_StreamSizes sizes{};
  auto status = ::QueryContextAttributes(&pipe.impl_,
    SECPKG_ATTR_STREAM_SIZES,
    &sizes
  );
  if (status == SEC_E_OK)
  {
    pipe.header_size_ = sizes.cbHeader;
    pipe.trailer_size_ = sizes.cbTrailer;
    pipe.max_message_size_ = sizes.cbMaximumMessage;
    return true;
  }
  error.assign(status, category());
  return false;
}


bool valid_certificate (pipe_t &pipe, std::error_code &error) noexcept
{
  if ((!pipe.factory_->mutual_auth_ && pipe.side_ == 'S')
    || !pipe.factory_->manual_certificate_check_)
  {
    return true;
  }

  certificate_t native_remote_certificate;
  auto status = ::QueryContextAttributes(&pipe.impl_,
    SECPKG_ATTR_REMOTE_CERT_CONTEXT,
    &native_remote_certificate.ref
  );

  bool is_valid = false;
  if (status == SEC_E_OK)
  {
    is_valid = pipe.factory_->manual_certificate_check_(
      sal::crypto::certificate_t::from_native_handle(
        std::move(native_remote_certificate)
      )
    );
    if (!is_valid)
    {
      error = std::make_error_code(std::errc::permission_denied);
    }
  }
  else
  {
    error.assign(status, category());
  }

  return is_valid;
}


void handle_io (pipe_t &pipe,
  SECURITY_STATUS status,
  buffer_t::list_t &in,
  buffer_t::list_t &out,
  std::error_code &error) noexcept
{
  std::cout << "\tIn\n";
  print_bufs(in);
  std::cout << "\tOut\n";
  print_bufs(out);

  // send/connected
  if (status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED)
  {
    error.clear();

    // send
    auto &data = out[0];
    if (data.pvBuffer && data.cbBuffer > 0)
    {
      auto first = static_cast<uint8_t *>(data.pvBuffer);
      auto last = first + data.cbBuffer;

      error.clear();
      pipe.event_handler_->pipe_on_send(first, last, error);
      if (error)
      {
        pipe.state_ = {};
        return;
      }
    }

    // extraneous data
    auto &extra = in[1];
    if (extra.BufferType == SECBUFFER_EXTRA
      && extra.pvBuffer
      && extra.cbBuffer > 0)
    {
      std::cout << pipe.side_ << "> EXTRA " << extra.cbBuffer << "\n";
    }
    else
    {
      pipe.recv_first_ = pipe.recv_last_ = nullptr;
    }

    if (status == SEC_E_OK)
    {
      std::cout << pipe.side_ << "> CONNECTED\n";
      if (!valid_certificate(pipe, error) || !finish_handshake(pipe, error))
      {
        pipe.state_ = {};
        return;
      }

      pipe.state_ = &pipe_t::exchange_messages;
      if (pipe.recv_first_ != pipe.recv_last_)
      {
        std::cout << pipe.side_ << "> MORE (" << (pipe.recv_last_ - pipe.recv_first_) << ")\n";
      }
    }
  }

  // need more
  else if (status == SEC_E_INCOMPLETE_MESSAGE)
  {
    error.clear();
  }

  // error
  else
  {
    pipe.state_ = {};
    error.assign(status, category());
  }
}


} // namespace


pipe_t::pipe_t (pipe_factory_ptr factory, bool stream) noexcept
  : factory_(factory)
{
  if (factory_->inbound_)
  {
    side_ = 'S';

    if (stream)
    {
      context_request_ |= ASC_REQ_STREAM;
    }
    else
    {
      context_request_ |= ASC_REQ_DATAGRAM;
    }
    if (factory_->mutual_auth_)
    {
      context_request_ |= ASC_REQ_MUTUAL_AUTH;
    }
  }
  else
  {
    side_ = 'C';

    //context_request_ = ISC_REQ_MANUAL_CRED_VALIDATION;
    if (stream)
    {
      context_request_ |= ISC_REQ_STREAM;
    }
    else
    {
      context_request_ |= ISC_REQ_DATAGRAM;
    }
    if (factory_->mutual_auth_)
    {
      context_request_ |= ISC_REQ_MUTUAL_AUTH;
    }
  }
}


pipe_t::~pipe_t () noexcept
{
  (void)::DeleteSecurityContext(&impl_);
}


void pipe_t::client_handshake (std::error_code &error) noexcept
{
  buffer_t::list_t in =
  {{
     token_t{recv_first_, recv_last_},
     empty_t{},
  }};

  uint8_t out_alert[1024];
  buffer_t::list_t out =
  {{
    token_t{send_first_, send_last_},
    alert_t{out_alert, out_alert + sizeof(out_alert)},
  }};

  print_flags(context_request_);
  auto status = call(::InitializeSecurityContext,
    &factory_->credentials_,            // phCredentials
    (is_valid() ? &impl_ : nullptr),    // phContext
    "sal.alt.ee",                       // pszTargetName
    context_request_,                   // fContextReq
    0,                                  // Reserved
    0,                                  // TargetDataRep
    (is_valid() ? &in : nullptr),       // pInput
    0,                                  // Reserved2
    &impl_,                             // phNewContext
    &out,                               // pOutput
    &context_flags_,                    // pfContextAttr
    nullptr                             // ptsExpiry
  );
  print_flags(context_flags_);

  handle_io(*this, status, in, out, error);
}


void pipe_t::server_handshake (std::error_code &error) noexcept
{
  buffer_t::list_t in =
  {{
    token_t{recv_first_, recv_last_},
    empty_t{},
    extra_t{},
  }};

  uint8_t out_alert[1024];
  buffer_t::list_t out =
  {{
    token_t{send_first_, send_last_},
    alert_t{out_alert, out_alert + sizeof(out_alert)},
  }};

  print_flags(context_request_);
  auto status = call(::AcceptSecurityContext,
    &factory_->credentials_,            // phCredentials
    (is_valid() ? &impl_ : nullptr),    // phContext
    &in,                                // pInput
    context_request_,                   // fContextReq
    0,                                  // TargetDataRep
    &impl_,                             // phNewContext
    &out,                               // pOutput
    &context_flags_,                    // pfContextAttr
    nullptr                             // ptsTimeStamp
  );
  print_flags(context_flags_);

  handle_io(*this, status, in, out, error);
}


void pipe_t::exchange_messages (std::error_code &error) noexcept
{
  error.clear();
}


#endif //}}}1


} // namespace crypto::__bits


__sal_end
