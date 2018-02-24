#pragma once

#include <sal/config.hpp>
#include <sal/crypto/certificate.hpp>
#include <functional>
#include <memory>

#if __sal_os_macos //{{{1
  #include <sal/__bits/ref.hpp>
  #include <Security/SecureTransport.h>
#endif //}}}1


__sal_begin


namespace crypto::__bits {


//
// channel_context_t
//

struct channel_context_t
{
  bool datagram = false;
  bool mutual_auth = false;
  crypto::certificate_t certificate{};
  std::function<bool(const crypto::certificate_t &)> certificate_check{};
};
using channel_context_ptr = std::shared_ptr<channel_context_t>;


//
// channel_t
//

struct channel_t
{
  channel_context_ptr context;
  const bool server;
  std::string peer_name{};
  std::error_code status = std::make_error_code(std::errc::not_connected);

#if __sal_os_macos //{{{1
  unique_ref<::SSLContextRef> handle{};
  void *syscall{};
#endif //}}}1

  channel_t (channel_context_ptr context, bool server) noexcept
    : context(context)
    , server(server)
  { }

  channel_t (const channel_t &) = delete;
  channel_t &operator= (const channel_t &) = delete;
};
using channel_ptr = std::unique_ptr<channel_t>;


} // namespace crypto::__bits


__sal_end
