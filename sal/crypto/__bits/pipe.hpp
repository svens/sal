#pragma once

#include <sal/config.hpp>
#include <sal/crypto/certificate.hpp>
#include <functional>
#include <memory>

#if __sal_os_macos
  #include <sal/__bits/ref.hpp>
  #include <Security/SecureTransport.h>
#elif __sal_os_windows
  #define SECURITY_WIN32
  #include <windows.h>
  #include <sspi.h>
  #include <vector>
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
  char side = '?';
  std::error_code handshake_result{};

#if __sal_os_macos

  unique_ref<SSLContextRef> context{};

#elif __sal_os_windows

  ::CtxtHandle context{};
  ULONG context_request{}, context_flags{};

  bool is_valid () const noexcept
  {
    return context.dwLower || context.dwUpper;
  }

  size_t complete_message_size{};
  std::vector<uint8_t> incomplete_message;
  bool buffer_while_incomplete_message ();

  size_t header_size, trailer_size, max_message_size = 8192;

#endif

  // I/O
  const uint8_t *in_first{};
  const uint8_t *in_last{};
  const uint8_t *in_ptr{};
  uint8_t *out_first{};
  uint8_t *out_last{};
  uint8_t *out_ptr{};


  pipe_t (pipe_factory_ptr factory, bool stream_oriented) noexcept
    : factory(factory)
    , stream_oriented(stream_oriented)
  {}

  ~pipe_t () noexcept;

  pipe_t (const pipe_t &) = delete;
  pipe_t &operator= (const pipe_t &) = delete;

  void ctor (std::error_code &error) noexcept;

  std::pair<size_t, size_t> handshake (std::error_code &error);
  std::pair<size_t, size_t> encrypt (std::error_code &error);
  std::pair<size_t, size_t> decrypt (std::error_code &error);


  std::pair<size_t, size_t> handshake (const uint8_t *ifirst,
    const uint8_t *ilast,
    uint8_t *ofirst,
    uint8_t *olast,
    std::error_code &error) noexcept
  {
    try
    {
      if (!handshake_result)
      {
        in_first = in_ptr = ifirst;
        in_last = ilast;
        out_first = out_ptr = ofirst;
        out_last = olast;
        return handshake(error);
      }
      error = handshake_result;
    }
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
    }
    return {};
  }


  std::pair<size_t, size_t> encrypt (const uint8_t *ifirst,
    const uint8_t *ilast,
    uint8_t *ofirst,
    uint8_t *olast,
    std::error_code &error) noexcept
  {
    try
    {
      if (handshake_result == std::errc::already_connected)
      {
        in_first = in_ptr = ifirst;
        in_last = ilast;
        out_first = out_ptr = ofirst;
        out_last = olast;
        return encrypt(error);
      }
      error = std::make_error_code(std::errc::not_connected);
    }
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
    }
    return {};
  }


  std::pair<size_t, size_t> decrypt (const uint8_t *ifirst,
    const uint8_t *ilast,
    uint8_t *ofirst,
    uint8_t *olast,
    std::error_code &error) noexcept
  {
    try
    {
      if (handshake_result == std::errc::already_connected)
      {
        in_first = in_ptr = ifirst;
        in_last = ilast;
        out_first = out_ptr = ofirst;
        out_last = olast;
        return decrypt(error);
      }
      error = std::make_error_code(std::errc::not_connected);
    }
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
    }
    return {};
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

#if __sal_os_windows
  ::CredHandle credentials{};
#endif

  pipe_factory_t (bool inbound)
    : inbound(inbound)
  {}

  ~pipe_factory_t () noexcept;

  void ctor (std::error_code &error) noexcept;

  pipe_ptr make_pipe (bool stream_oriented)
  {
    return std::make_unique<pipe_t>(shared_from_this(), stream_oriented);
  }
};


} // namespace crypto::__bits


__sal_end
