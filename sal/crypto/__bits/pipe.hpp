#pragma once

#include <sal/config.hpp>
#include <sal/crypto/certificate.hpp>
#include <functional>
#include <memory>

#if __sal_os_macos
  #include <sal/__bits/ref.hpp>
  #include <Security/SecureTransport.h>
#endif


__sal_begin


namespace crypto::__bits {


struct pipe_factory_t;
using pipe_factory_ptr = std::shared_ptr<pipe_factory_t>;


//
// pipe_t
//

struct pipe_t
{
  pipe_factory_ptr factory;
  bool stream_oriented;
  std::string peer_name{};

#if __sal_os_macos
  unique_ref<SSLContextRef> context{};
#endif

  // I/O
  const uint8_t *recv_first{};
  const uint8_t *recv_last{};
  uint8_t *send_first{};
  uint8_t *send_last{};
  uint8_t *send_ptr{};


  pipe_t (pipe_factory_ptr factory, bool stream_oriented) noexcept
    : factory(factory)
    , stream_oriented(stream_oriented)
  {}

  void ctor (std::error_code &error) noexcept;

  void (pipe_t::*state)(std::error_code &) noexcept = {};
  void client_handshake (std::error_code &error) noexcept;
  void server_handshake (std::error_code &error) noexcept;
  void connected (std::error_code &error) noexcept;

  void process (std::error_code &error) noexcept
  {
    (this->*state)(error);
  }

  pipe_t (const pipe_t &) = delete;
  pipe_t &operator= (const pipe_t &) = delete;
};
using pipe_ptr = std::unique_ptr<pipe_t>;


//
// pipe_factory_t
//

struct pipe_factory_t
  : public std::enable_shared_from_this<pipe_factory_t>
{
  const bool inbound;
  bool mutual_auth = false;
  certificate_t certificate{};
  std::function<bool(const crypto::certificate_t &)> certificate_check{};

  pipe_factory_t (bool inbound)
    : inbound(inbound)
  {}

  void ctor (std::error_code &error) noexcept;

  pipe_ptr make_pipe (bool stream_oriented)
  {
    return std::make_unique<pipe_t>(shared_from_this(), stream_oriented);
  }
};


} // namespace crypto::__bits


__sal_end
