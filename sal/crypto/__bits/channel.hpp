#pragma once

#include <sal/config.hpp>
#include <sal/crypto/certificate.hpp>
#include <functional>
#include <memory>

#if __sal_os_linux //{{{1
  // TODO
#elif __sal_os_macos //{{{1
  #include <sal/__bits/ref.hpp>
  #include <Security/SecureTransport.h>
#elif __sal_os_windows //{{{1
  #define SECURITY_WIN32
  #include <windows.h>
  #include <sspi.h>
  #include <vector>
#endif //}}}1


__sal_begin


namespace crypto::__bits {


//
// channel_context_t
//

struct channel_context_t
{
  const bool datagram;
  const bool server;
  certificate_t certificate{};
  std::function<bool(const crypto::certificate_t &)> certificate_check{};

#if __sal_os_linux //{{{1
  // TODO
#elif __sal_os_macos //{{{1
  // none
#elif __sal_os_windows //{{{1
  ::CredHandle credentials;
#endif //}}}1

  channel_context_t (bool datagram, bool server) noexcept
    : datagram(datagram)
    , server(server)
  { }

  void ctor (std::error_code &error) noexcept;

  ~channel_context_t () noexcept;

  channel_context_t (const channel_context_t &) = delete;
  channel_context_t &operator= (const channel_context_t &) = delete;
};
using channel_context_ptr = std::shared_ptr<channel_context_t>;


//
// channel_t
//

struct channel_t
{
  channel_context_ptr context;
  bool mutual_auth = false;
  std::string peer_name{};
  std::error_code handshake_status = std::make_error_code(std::errc::not_connected);

#if __sal_os_linux //{{{1

  // TODO

#elif __sal_os_macos //{{{1

  unique_ref<::SSLContextRef> handle{};
  void *syscall{};

#elif __sal_os_windows //{{{1

  ::CtxtHandle handle, *handle_p{};
  ULONG context_request{}, context_flags{};
  size_t header_size{}, trailer_size{}, max_message_size = 8192;

  size_t complete_message_size{};
  std::vector<uint8_t> in{};

  std::unique_ptr<void, decltype(&::FreeContextBuffer)> out{nullptr,
    &::FreeContextBuffer
  };
  const uint8_t *out_ptr{};
  size_t out_size{};

#endif //}}}1

  channel_t (channel_context_ptr context) noexcept
    : context(context)
  { }

  void ctor (std::error_code &error) noexcept;

  ~channel_t () noexcept;

  channel_t (const channel_t &) = delete;
  channel_t &operator= (const channel_t &) = delete;
};
using channel_ptr = std::unique_ptr<channel_t>;


} // namespace crypto::__bits


__sal_end
