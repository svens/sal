#pragma once

#include <sal/config.hpp>
#include <sal/crypto/certificate.hpp>
#include <functional>
#include <memory>

#if __sal_os_macos
  #include <sal/__bits/ref.hpp>
  #include <Security/SecureTransport.h>
#endif


#include <iostream>


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
  char side = '?';
  std::error_code handshake_result{};

#if __sal_os_macos
  unique_ref<SSLContextRef> context{};
#endif

  // I/O
  const uint8_t *in_first{};
  const uint8_t *in_last{};
  uint8_t *out_first{};
  uint8_t *out_last{};
  uint8_t *out_ptr{};


  pipe_t (pipe_factory_ptr factory, bool stream_oriented) noexcept
    : factory(factory)
    , stream_oriented(stream_oriented)
  {}

  pipe_t (const pipe_t &) = delete;
  pipe_t &operator= (const pipe_t &) = delete;

  void ctor (std::error_code &error) noexcept;

  void handshake (std::error_code &error) noexcept;
  void encrypt (std::error_code &error) noexcept;
  void decrypt (std::error_code &error) noexcept;


  std::pair<size_t, size_t> handshake (const uint8_t *in_first,
    const uint8_t *in_last,
    uint8_t *out_first,
    uint8_t *out_last,
    std::error_code &error) noexcept
  {
    this->in_first = in_first;
    this->in_last = in_last;
    this->out_first = out_ptr = out_first;
    this->out_last = out_last;
    handshake(error);
    return {this->in_first - in_first, out_ptr - this->out_first};
  }


  std::pair<size_t, size_t> encrypt (const uint8_t *in_first,
    const uint8_t *in_last,
    uint8_t *out_first,
    uint8_t *out_last,
    std::error_code &error) noexcept
  {
    this->in_first = in_first;
    this->in_last = in_last;
    this->out_first = out_ptr = out_first;
    this->out_last = out_last;
    encrypt(error);
    return {this->in_first - in_first, out_ptr - this->out_first};
  }


  std::pair<size_t, size_t> decrypt (const uint8_t *in_first,
    const uint8_t *in_last,
    uint8_t *out_first,
    uint8_t *out_last,
    std::error_code &error) noexcept
  {
    this->in_first = in_first;
    this->in_last = in_last;
    this->out_first = out_ptr = out_first;
    this->out_last = out_last;
    decrypt(error);
    return {this->in_first - in_first, out_ptr - this->out_first};
  }
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
