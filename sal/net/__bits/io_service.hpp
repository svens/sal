#pragma once

#include <sal/config.hpp>
#include <sal/net/__bits/socket.hpp>
#include <sal/intrusive_queue.hpp>
#include <array>
#include <chrono>


__sal_begin


namespace net { namespace __bits {


#if __sal_os_windows


struct io_context_t;


struct io_buf_t
  : public OVERLAPPED
{
  char *begin, *end;
  uintptr_t user_data;
  size_t request_id;
  DWORD transferred;
  std::error_code error;

  io_context_t *context;
  no_sync_t::intrusive_queue_hook_t completed;


  io_buf_t () noexcept
    : OVERLAPPED{0}
  {}


  WSABUF to_buf () const noexcept
  {
    WSABUF buf;
    buf.len = static_cast<ULONG>(end - begin);
    buf.buf = begin;
    return buf;
  }


  void io_result (int result) noexcept;
};


struct async_receive_t
  : public io_buf_t
{
  void start (socket_t &socket, message_flags_t flags) noexcept;
};


struct async_receive_from_t
  : public io_buf_t
{
  sockaddr_storage address;
  INT address_size;

  void start (socket_t &socket, message_flags_t flags) noexcept;
};


struct async_send_to_t
  : public io_buf_t
{
  void start (socket_t &socket,
    const void *address, size_t address_size,
    message_flags_t flags
  ) noexcept;
};


struct async_send_t
  : public io_buf_t
{
  void start (socket_t &socket, message_flags_t flags) noexcept;
};


struct async_connect_t
  : public io_buf_t
{
  native_socket_t handle;
  bool finished;

  void start (socket_t &socket, const void *address, size_t address_size)
    noexcept;

  void finish (std::error_code &error) noexcept;
};


struct async_accept_t
  : public io_buf_t
{
  native_socket_t accepted, acceptor;
  sockaddr_storage *local_address, *remote_address;
  bool finished;

  void start (socket_t &socket, int family) noexcept;
  void finish (std::error_code &error) noexcept;
};


struct io_service_t
{
  HANDLE iocp = INVALID_HANDLE_VALUE;
  static constexpr size_t max_completion_count = 1024;

  io_service_t (std::error_code &error) noexcept;
  ~io_service_t () noexcept;

  void associate (socket_t &socket, std::error_code &error) noexcept;
};


struct io_context_t
{
  io_service_t &io_service;
  std::array<OVERLAPPED_ENTRY, io_service_t::max_completion_count> completions;
  ULONG max_completion_count, completion_count = 0, completion_index = 0;
  intrusive_queue_t<io_buf_t, no_sync_t, &io_buf_t::completed> immediate_completions{};


  io_context_t (io_service_t &io_service, size_t max_completion_count) noexcept
    : io_service(io_service)
    , max_completion_count(static_cast<ULONG>(max_completion_count))
  {}

  io_buf_t *try_get () noexcept;

  io_buf_t *get (const std::chrono::milliseconds &timeout,
    std::error_code &error
  ) noexcept;
};


#endif // __sal_os_windows


}} // namespace net::__bits


__sal_end
