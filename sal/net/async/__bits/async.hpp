#pragma once

#include <sal/config.hpp>
#include <sal/intrusive_mpsc_queue.hpp>
#include <sal/intrusive_queue.hpp>
#include <sal/net/__bits/socket.hpp>
#include <sal/spinlock.hpp>
#include <algorithm>
#include <array>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>


__sal_begin


namespace net::async::__bits {


using net::__bits::socket_t;
using net::__bits::message_flags_t;

struct handler_t;


enum class op_t
{
  receive_from,
  receive,
  send_to,
  send,
  accept,
  connect,
};


struct io_base_t //{{{1
{
#if __sal_os_windows
  OVERLAPPED overlapped{};
  using transferred_size_t = DWORD;
  using endpoint_size_t = INT;
#elif __sal_os_linux || __sal_os_macos
  using transferred_size_t = size_t;
  using endpoint_size_t = size_t;
#endif

  op_t op{};

  union
  {
    struct
    {
      transferred_size_t transferred;
      void *remote_endpoint;
      endpoint_size_t remote_endpoint_capacity;
      message_flags_t *flags;
    } receive_from;

    struct
    {
      transferred_size_t transferred;
      message_flags_t *flags;
    } receive;

    struct
    {
      transferred_size_t transferred;
#if __sal_os_linux || __sal_os_macos
      message_flags_t flags;
      sockaddr_storage remote_endpoint;
      size_t remote_endpoint_size;
#endif
    } send_to;

    struct
    {
      transferred_size_t transferred;
    } send;

    struct
    {
      socket_t::handle_t *socket_handle;
      size_t unused;
    } accept;

    struct
    {
      size_t unused;
    } connect;
  } pending{};

  handler_t *current_owner{};
  uintptr_t context_type{};
  void *context{};

  uint8_t result[160];
  size_t *transferred{};
  std::error_code status{};

  uint8_t *begin{}, *end{};

  union
  {
    intrusive_mpsc_queue_hook_t<io_base_t> free{};
    intrusive_mpsc_queue_hook_t<io_base_t> completed;
    intrusive_queue_hook_t<io_base_t> pending_io;
  };
  using free_list_t = intrusive_mpsc_queue_t<&io_base_t::free>;
  using completed_list_t = intrusive_mpsc_queue_t<&io_base_t::completed>;
  using pending_io_list_t = intrusive_queue_t<&io_base_t::pending_io>;

  free_list_t &free_list;


  io_base_t (free_list_t &free_list) noexcept
    : free_list(free_list)
  { }


  io_base_t (const io_base_t &) = delete;
};


struct io_t //{{{1
  : public io_base_t
{
  static constexpr size_t data_size = 2048 - sizeof(io_base_t);
  uint8_t data[data_size];


  io_t (free_list_t &free_list) noexcept
    : io_base_t(free_list)
  { }


  // should be deleted but keeping here for easier service_t::alloc_io new
  // batch allocation and free_list insertion
  io_t (const io_t &io) noexcept
    : io_base_t(io.free_list)
  {
    free_list.push(this);
  }


  void reset () noexcept
  {
    begin = data;
    end = data + sizeof(data);
  }
};
static_assert(sizeof(io_t) == 2048);
static_assert(io_t::data_size > 1500, "io_t::data_size less than MTU size");
static_assert(std::is_trivially_destructible_v<io_t>);


struct io_deleter_t //{{{1
{
  void operator() (io_t *io) noexcept
  {
    io->free_list.push(io);
  }
};
using io_ptr = std::unique_ptr<io_t, io_deleter_t>;


struct service_t //{{{1
{
#if __sal_os_windows
  HANDLE iocp;
#elif __sal_os_linux || __sal_os_macos
  int queue;
#endif

  std::mutex io_pool_mutex{};
  std::deque<std::vector<io_t>> io_pool{};
  io_t::free_list_t free_list{};
  size_t io_pool_size{};

  sal::spinlock_t completed_mutex{};
  io_t::completed_list_t completed_list{};


  service_t ();
  ~service_t () noexcept;


  io_t *alloc_io ()
  {
    std::lock_guard lock(io_pool_mutex);
    auto io = free_list.try_pop();
    if (!io)
    {
      auto batch_size = 16 * (1ULL << io_pool.size());
      io_pool.emplace_back(batch_size, free_list);
      io_pool_size += batch_size;
      io = free_list.try_pop();
    }
    return static_cast<io_t *>(io);
  }


  io_t *make_io (void *context, uintptr_t context_type)
  {
    auto io = alloc_io();
    io->current_owner = nullptr;
    io->context_type = context_type;
    io->context = context;
    io->reset();
    return io;
  }


  void enqueue (io_t *io) noexcept
  {
    completed_list.push(io);
  }


  io_t *dequeue () noexcept
  {
    std::lock_guard lock(completed_mutex);
    return static_cast<io_t *>(completed_list.try_pop());
  }


  bool wait_for_more (const std::chrono::milliseconds &timeout,
    std::error_code &error
  ) noexcept;


  io_t *poll (const std::chrono::milliseconds &timeout, std::error_code &error)
    noexcept
  {
    do
    {
      if (auto io = dequeue())
      {
        return io;
      }
    } while (wait_for_more(timeout, error));

    return {};
  }
};
using service_ptr = std::shared_ptr<service_t>;


struct handler_t //{{{1
{
  service_ptr service;
  socket_t socket;

  uintptr_t context_type{};
  void *context{};


  handler_t (service_ptr service, socket_t &socket, std::error_code &error) noexcept;
  ~handler_t () noexcept;

  handler_t (const handler_t &) = delete;
  handler_t &operator= (const handler_t &) = delete;


#if __sal_os_linux || __sal_os_macos

  #if __sal_os_linux
    uint32_t await_events{};
  #endif

  struct pending_t
  {
    std::mutex mutex{};
    io_t::pending_io_list_t list{};
  } pending_read{}, pending_write{};


  void start (io_t *io, pending_t &pending) noexcept
  {
    std::lock_guard lock(pending.mutex);
    if (pending.list.empty() && try_finish(io, lock))
    {
      service->enqueue(io);
    }
    else
    {
      pending.list.push(io);
    }
  }


  bool try_finish (io_t *io,
    const std::lock_guard<std::mutex> &pending_list_lock
  ) noexcept;

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


//}}}1


} // namespace net::async::__bits


__sal_end
