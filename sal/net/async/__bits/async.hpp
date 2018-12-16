#pragma once

#include <sal/config.hpp>
#include <sal/intrusive_mpsc_queue.hpp>
#include <sal/intrusive_queue.hpp>
#include <sal/net/__bits/socket.hpp>
#include <deque>
#include <memory>
#include <mutex>


__sal_begin


namespace net::async::__bits {


using net::__bits::socket_t;
using net::__bits::message_flags_t;

struct io_t;
struct service_t;
struct completion_queue_t;
struct handler_t;


struct io_base_t //{{{1
{
#if __sal_os_windows
  OVERLAPPED overlapped{};
  using endpoint_size_t = INT;
  bool (*on_finish)(io_t *) noexcept = nullptr;
#elif __sal_os_linux || __sal_os_macos
  using endpoint_size_t = size_t;
  bool (*on_finish)(io_t *, uint16_t, uint32_t) noexcept = nullptr;
#endif

  union
  {
    struct
    {
      void *remote_endpoint;
      endpoint_size_t remote_endpoint_capacity;
    } receive_from;

    struct
    {
    } receive;

    struct
    {
      message_flags_t flags;
      sockaddr_storage remote_endpoint;
      size_t remote_endpoint_size;
    } send_to;

    struct
    {
      message_flags_t flags;
    } send;

    struct
    {
      socket_t::handle_t *socket_handle;
      size_t unused_transferred;
      message_flags_t unused_flags;
    } accept;

    struct
    {
      size_t unused_transferred;
      message_flags_t unused_flags;
    } connect;
  } pending{};

  handler_t *owner{};
  uintptr_t context_type{};
  void *context{};

  uint64_t op{};
  std::byte result[160];
  size_t *transferred{};
  message_flags_t *flags{};
  std::error_code status{};

  std::byte *begin{};
  const std::byte *end{};

  union
  {
    intrusive_mpsc_queue_hook_t<io_base_t> completed_io{};
    intrusive_queue_hook_t<io_base_t> pending_io;
  };
  using completed_list_t = intrusive_mpsc_queue_t<&io_base_t::completed_io>;
  using pending_list_t = intrusive_queue_t<&io_base_t::pending_io>;

  service_t &service;
  completed_list_t *completed_list;


  io_base_t (service_t &service, completed_list_t *completed_list) noexcept
    : service(service)
    , completed_list(completed_list)
  { }


  void completed () noexcept
  {
    completed_list->push(this);
  }


  void completed (completed_list_t &list) noexcept;


  io_base_t () = delete;
  io_base_t (const io_base_t &) = delete;
  io_base_t &operator= (const io_base_t &) = delete;
  io_base_t (io_base_t &&) = delete;
  io_base_t &operator= (io_base_t &&) = delete;
};


struct io_t //{{{1
  : public io_base_t
{
  static constexpr size_t mtu_size = 1500;
  static constexpr size_t data_size = 2048 - sizeof(io_base_t);
  static_assert(data_size >= mtu_size);

  std::byte data[data_size];

  io_t (service_t &service, completed_list_t *completed_list) noexcept
    : io_base_t(service, completed_list)
  { }
};

static_assert(sizeof(io_t) == 2048);
static_assert(std::is_trivially_destructible_v<io_t>);


struct service_t //{{{1
{
#if __sal_os_windows
  HANDLE iocp;
#elif __sal_os_linux || __sal_os_macos
  int queue;
#endif

  std::mutex io_pool_mutex{};
  std::deque<std::unique_ptr<std::byte[]>> io_pool{};
  size_t io_pool_size{};

  std::mutex completed_list_mutex{};
  io_t::completed_list_t completed_list{}, free_list{};


  service_t ();
  ~service_t () noexcept;


  io_t *make_io (io_t::completed_list_t *completed_list);


  io_t *make_io ()
  {
    return make_io(&completed_list);
  }


  io_t *try_get () noexcept
  {
    std::lock_guard lock(completed_list_mutex);
    return static_cast<io_t *>(completed_list.try_pop());
  }


  service_t (const service_t &) = delete;
  service_t &operator= (const service_t &) = delete;
  service_t (service_t &&) = delete;
  service_t &operator= (service_t &&) = delete;
};
using service_ptr = std::shared_ptr<service_t>;


struct completion_queue_t //{{{1
{
  service_ptr service;
  io_t::completed_list_t completed_list{};


  completion_queue_t (service_ptr service) noexcept
    : service(service)
  { }


  ~completion_queue_t () noexcept
  {
    while (auto io = completed_list.try_pop())
    {
      io->completed(service->completed_list);
    }
  }


  io_t *make_io ()
  {
    return service->make_io(&completed_list);
  }


  io_t *try_get () noexcept
  {
    if (auto io = static_cast<io_t *>(completed_list.try_pop()))
    {
      return io;
    }
    return service->try_get();
  }


  bool wait (const std::chrono::milliseconds &timeout,
    std::error_code &error
  ) noexcept;


  completion_queue_t () = delete;
  completion_queue_t (const completion_queue_t &) = delete;
  completion_queue_t &operator= (const completion_queue_t &) = delete;
  completion_queue_t (completion_queue_t &&) = delete;
  completion_queue_t &operator= (completion_queue_t &&) = delete;
};


struct handler_t //{{{1
{
  service_ptr service;
  socket_t socket;

  uintptr_t context_type{};
  void *context{};


  handler_t (service_ptr service, socket_t &socket, std::error_code &error) noexcept;
  ~handler_t () noexcept;


#if __sal_os_linux || __sal_os_macos

  #if __sal_os_linux
    uint32_t await_events{};
  #endif

  struct pending_t
  {
    std::mutex mutex{};
    io_t::pending_list_t list{};
  } pending_read{}, pending_write{};

#endif


  void start_receive_from (io_t *io,
    void *remote_endpoint,
    size_t remote_endpoint_capacity,
    size_t *transferred,
    message_flags_t *flags
  ) noexcept;


  void start_receive (io_t *io,
    size_t *transferred,
    message_flags_t *flags
  ) noexcept;


  void start_send_to (io_t *io,
    const void *remote_endpoint,
    size_t remote_endpoint_size,
    size_t *transferred,
    message_flags_t flags
  ) noexcept;


  void start_send (io_t *io,
    size_t *transferred,
    message_flags_t flags
  ) noexcept;


  void start_accept (io_t *io,
    int family,
    socket_t::handle_t *socket_handle
  ) noexcept;


  void start_connect (io_t *io,
    const void *remote_endpoint,
    size_t remote_endpoint_size
  ) noexcept;


  handler_t () = delete;
  handler_t (const handler_t &) = delete;
  handler_t &operator= (const handler_t &) = delete;
  handler_t (handler_t &&) = delete;
  handler_t &operator= (handler_t &&) = delete;
};
using handler_ptr = std::unique_ptr<handler_t>;


inline handler_ptr make_handler (service_ptr service,
  socket_t &socket,
  std::error_code &error) noexcept
{
  auto handler = handler_ptr(
    new(std::nothrow) handler_t(service, socket, error)
  );
  if (!handler)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  else if (error)
  {
    handler.reset();
  }
  return handler;
}


inline void io_base_t::completed (completed_list_t &list) noexcept
{
  if (completed_list != &service.free_list)
  {
    completed_list = &list;
  }
  completed();
}


//}}}1


} // namespace net::async::__bits


__sal_end
