#pragma once

#include <sal/config.hpp>
#include <sal/crypto/certificate.hpp>
#include <functional>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

#if __sal_os_macos //{{{1
#elif __sal_os_linux //{{{1
#elif __sal_os_windows //{{{1
  #define SECURITY_WIN32
  #include <windows.h>
  #include <sspi.h>
#endif //}}}1


__sal_begin


namespace crypto { namespace __bits {


struct pipe_factory_t
{
#if __sal_os_macos
#elif __sal_os_linux
#elif __sal_os_windows
  ::CredHandle credentials_{};
#endif

  // options
  bool inbound_{};
  bool mutual_auth_{};
  bool no_certificate_check_{};
  std::function<bool(const crypto::certificate_t &)> manual_certificate_check_{};
  certificate_t certificate_{};

  void ctor (std::error_code &error) noexcept;
  ~pipe_factory_t () noexcept;

  pipe_factory_t () = default;
  pipe_factory_t (const pipe_factory_t &) = delete;
  pipe_factory_t &operator= (const pipe_factory_t &) = delete;
};
using pipe_factory_ptr = std::shared_ptr<pipe_factory_t>;


struct pipe_event_handler_t
{
  virtual void pipe_on_send (const uint8_t *first,
    const uint8_t *last,
    std::error_code &error
  ) noexcept = 0;
};


struct pipe_t
{
#if __sal_os_macos
#elif __sal_os_linux
#elif __sal_os_windows
  ::CtxtHandle impl_{};
  ULONG context_request_{}, context_flags_{};
  size_t header_size_{}, trailer_size_{}, max_message_size_{};

  bool is_valid () const noexcept
  {
    return impl_.dwLower || impl_.dwUpper;
  }
#endif

  pipe_factory_ptr factory_;
  char side_;

  // I/O
  pipe_event_handler_t *event_handler_ = {};
  uint8_t *send_first_{}, *send_last_{}, *recv_first_{}, *recv_last_{};

  pipe_t (pipe_factory_ptr factory, bool stream) noexcept;
  ~pipe_t () noexcept;


  void start_handshake (pipe_event_handler_t *handler,
    uint8_t *send_buffer_first,
    uint8_t *send_buffer_last,
    std::error_code &error) noexcept
  {
    event_handler_ = handler;
    send_first_ = send_buffer_first;
    send_last_ = send_buffer_last;

    if (factory_->inbound_)
    {
      state_ = &pipe_t::server_handshake;
      error.clear();
    }
    else
    {
      state_ = &pipe_t::client_handshake;
      client_handshake(error);
    }
  }


  void process (const uint8_t *first, const uint8_t *last, std::error_code &error)
    noexcept
  {
    // const_cast is ok: we don't change and native TLS call promises same
    recv_first_ = const_cast<uint8_t *>(first);
    recv_last_ = const_cast<uint8_t *>(last);
    (this->*state_)(error);
  }


  // TLS
  //                   client       server
  //                   -------------------
  //
  // client_handshake / server_handshake
  // -----------------------------------
  //        client_send_hello  -->  server_wait_client_hello
  // client_wait_server_hello  <--  server_send_hello
  //   [client_verify_server]
  // client_send_key_exchange  -->  server_wait_client_key_exchange
  //                                [server_verify_client]
  //          client_finished  <->  server_finished
  //
  // exchange_messages
  // -----------------
  //        exchange_messages  <->  exchange_messages

  // state handler
  void (pipe_t::*state_)(std::error_code &error) noexcept = {};
  void client_handshake (std::error_code &error) noexcept;
  void server_handshake (std::error_code &error) noexcept;
  void exchange_messages (std::error_code &error) noexcept;

  pipe_t (const pipe_t &) = delete;
  pipe_t &operator= (const pipe_t &) = delete;
};
using pipe_ptr = std::unique_ptr<pipe_t>;


}} // namespace crypto::__bits


__sal_end
