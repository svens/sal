#pragma once

#include <sal/config.hpp>
#include <sal/net/__bits/socket.hpp>
#include <sal/intrusive_queue.hpp>
#include <array>
#include <chrono>

#if __sal_os_darwin
  #include <sys/types.h>
  #include <sys/event.h>
#endif


__sal_begin


namespace net { namespace __bits {


struct io_context_t;


struct io_buf_base_t
{
  io_context_t *context{};
  uintptr_t request_id{}, user_data{};
  char *begin{}, *end{};
  std::error_code error{};
};


template <typename T>
struct async_operation_t
{
#if _MSC_VER

  static uintptr_t type_id () noexcept
  {
    static const T *p = nullptr;
    return reinterpret_cast<uintptr_t>(&p);
  }

#else

  static constexpr uintptr_t type_id () noexcept
  {
    return reinterpret_cast<uintptr_t>(&async_operation_t::type_id);
  }

#endif
};


#if __sal_os_windows


struct io_buf_t
  : public OVERLAPPED
  , public io_buf_base_t
{
  DWORD transferred;

  no_sync_t::intrusive_queue_hook_t completed;
  using completed_queue_t = intrusive_queue_t<io_buf_t, no_sync_t, &io_buf_t::completed>;


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
  , public async_operation_t<async_receive_t>
{
  void start (socket_t &socket, message_flags_t flags) noexcept;
};


struct async_receive_from_t
  : public io_buf_t
  , public async_operation_t<async_receive_from_t>
{
  sockaddr_storage address;
  INT address_size;

  void start (socket_t &socket, message_flags_t flags) noexcept;
};


struct async_send_to_t
  : public io_buf_t
  , public async_operation_t<async_send_to_t>
{
  void start (socket_t &socket,
    const void *address, size_t address_size,
    message_flags_t flags
  ) noexcept;
};


struct async_send_t
  : public io_buf_t
  , public async_operation_t<async_send_t>
{
  void start (socket_t &socket, message_flags_t flags) noexcept;
};


struct async_connect_t
  : public io_buf_t
  , public async_operation_t<async_connect_t>
{
  native_socket_t handle;
  bool finished;

  void start (socket_t &socket, const void *address, size_t address_size)
    noexcept;

  void finish (std::error_code &error) noexcept;
};


struct async_accept_t
  : public io_buf_t
  , public async_operation_t<async_accept_t>
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
  static constexpr size_t max_events_per_wait = 1024;

  io_service_t (std::error_code &error) noexcept;
  ~io_service_t () noexcept;

  void associate (socket_t &socket, std::error_code &error) noexcept;
};


struct io_context_t
{
  io_service_t &io_service;

  using event_list_t = std::array<OVERLAPPED_ENTRY, io_service_t::max_events_per_wait>;

  ULONG max_events_per_wait;
  event_list_t completions;
  event_list_t::iterator completion{}, last_completion{};
  io_buf_t::completed_queue_t completed{};


  io_context_t (io_service_t &io_service, size_t max_events_per_wait) noexcept
    : io_service(io_service)
    , max_events_per_wait(static_cast<ULONG>(max_events_per_wait))
  {}

  io_buf_t *try_get () noexcept;

  io_buf_t *get (const std::chrono::milliseconds &timeout,
    std::error_code &error
  ) noexcept;
};


#elif __sal_os_darwin


struct io_buf_t
  : public io_buf_base_t
{
  size_t transferred{};

  union
  {
    mpsc_sync_t::intrusive_queue_hook_t completed{}, pending_receive, pending_send;
  };
  using completed_queue_t = intrusive_queue_t<
    io_buf_t, mpsc_sync_t, &io_buf_t::completed
  >;
  using pending_receive_queue_t = intrusive_queue_t<
    io_buf_t, mpsc_sync_t, &io_buf_t::pending_receive
  >;
  using pending_send_queue_t = intrusive_queue_t<
    io_buf_t, mpsc_sync_t, &io_buf_t::pending_send
  >;

  bool retry_receive (socket_t &socket) noexcept;
  bool retry_send (socket_t &socket) noexcept;
};


struct async_receive_from_t
  : public io_buf_t
  , public async_operation_t<async_receive_from_t>
{
  message_flags_t flags;
  sockaddr_storage address;
  size_t address_size;

  void start (socket_t &socket, message_flags_t flags) noexcept;
};


struct async_receive_t
  : public io_buf_t
  , public async_operation_t<async_receive_t>
{
  message_flags_t flags;

  void start (socket_t &socket, message_flags_t flags) noexcept;
};


struct async_send_to_t
  : public io_buf_t
  , public async_operation_t<async_send_to_t>
{
  sockaddr_storage address;
  size_t address_size;
  message_flags_t flags;

  void start (socket_t &socket,
    const void *address, size_t address_size,
    message_flags_t flags
  ) noexcept;
};


struct async_send_t
  : public io_buf_t
  , public async_operation_t<async_send_t>
{
  message_flags_t flags;

  void start (socket_t &socket, message_flags_t flags) noexcept;
};


struct io_service_t
{
  int queue;
  static constexpr size_t max_events_per_wait = 1024;

  io_service_t (std::error_code &error) noexcept;
  ~io_service_t () noexcept;

  void associate (socket_t &socket, std::error_code &error) noexcept;
};


struct io_context_t
{
  io_service_t &io_service;

  using event_list_t = std::array<struct ::kevent, io_service_t::max_events_per_wait>;

  size_t max_events_per_wait;
  event_list_t events{};
  event_list_t::iterator event{}, last_event{};
  io_buf_t::completed_queue_t completed{};


  io_context_t (io_service_t &io_service, size_t max_events_per_wait) noexcept
    : io_service(io_service)
    , max_events_per_wait(max_events_per_wait)
  {}


  void ready (io_buf_t *io_buf) noexcept
  {
    io_buf->context = this;
    completed.push(io_buf);
  }


  io_buf_t *retry_receive (socket_t &socket) noexcept;
  io_buf_t *retry_send (socket_t &socket) noexcept;

  io_buf_t *try_get () noexcept;

  io_buf_t *get (const std::chrono::milliseconds &timeout,
    std::error_code &error
  ) noexcept;
};


#endif


}} // namespace net::__bits


__sal_end
