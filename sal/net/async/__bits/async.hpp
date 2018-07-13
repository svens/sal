#pragma once

#include <sal/config.hpp>
#include <sal/intrusive_mpsc_queue.hpp>
#include <sal/net/__bits/socket.hpp>
#include <sal/spinlock.hpp>
#include <sal/type_id.hpp>
#include <algorithm>
#include <array>
#include <deque>
#include <memory>
#include <mutex>


__sal_begin


namespace net::async::__bits {


struct io_block_t;
struct async_socket_t;


struct io_base_t //{{{1
{
  io_block_t &block;

  uintptr_t context_type{};
  void *context{};

  async_socket_t *socket{};
  uintptr_t result_type{};
  uint8_t result_data[160];
  size_t *transferred{};
  std::error_code status{};

  uint8_t *begin{}, *end{};

  std::atomic<size_t> *outstanding{};

  union
  {
    intrusive_mpsc_queue_hook_t<io_base_t> free{};
    intrusive_mpsc_queue_hook_t<io_base_t> error;
  };
  using free_list_t = intrusive_mpsc_queue_t<&io_base_t::free>;
  using error_queue_t = intrusive_mpsc_queue_t<&io_base_t::error>;


  io_base_t (io_block_t &block) noexcept
    : block(block)
  {}
};


struct io_t //{{{1
  : public io_base_t
{
  static constexpr size_t data_size = 2048 - sizeof(io_base_t);
  uint8_t data[data_size];


  io_t (io_block_t &block) noexcept
    : io_base_t(block)
  {}
};
static_assert(sizeof(io_t) == 2048);
static_assert(io_t::data_size > 1500, "io_t::data_size less than MTU size");


struct io_block_t //{{{1
{
  io_t::free_list_t &free_list;
  std::unique_ptr<char[]> data;

#if __sal_os_windows
  RIO_BUFFERID buffer_id;
#endif

  io_block_t (size_t size, io_t::free_list_t &free_list);
  ~io_block_t () noexcept;
};


struct io_deleter_t //{{{1
{
  void operator() (io_t *io) noexcept
  {
    io->block.free_list.push(io);
  }
};
using io_ptr = std::unique_ptr<io_t, io_deleter_t>;


struct service_t //{{{1
{
#if __sal_os_windows
  OVERLAPPED overlapped{};
  HANDLE iocp;
#endif

  std::mutex io_pool_mutex{};
  std::deque<io_block_t> io_pool{};
  io_t::free_list_t free_list{};
  size_t io_pool_size{0};

  sal::spinlock_t error_queue_mutex{};
  io_t::error_queue_t error_queue{};


  service_t ();
  ~service_t () noexcept;


  io_t *alloc_io ()
  {
    std::lock_guard lock(io_pool_mutex);
    if (auto io = static_cast<io_t *>(free_list.try_pop()))
    {
      return io;
    }
    size_t block_size = 512 * sizeof(io_t) * (1ULL << io_pool.size());
    io_pool.emplace_back(block_size, free_list);
    io_pool_size += block_size;
    return static_cast<io_t *>(free_list.try_pop());
  }


  io_t *make_io (void *context = nullptr,
    uintptr_t context_type = type_v<std::nullptr_t>)
  {
    auto io = alloc_io();
    io->begin = io->data;
    io->end = io->data + sizeof(io->data);
    io->context_type = context_type;
    io->context = context;
    io->socket = nullptr;
    return io;
  }


  io_t *dequeue_error () noexcept
  {
    std::lock_guard lock(error_queue_mutex);
    return static_cast<io_t *>(error_queue.try_pop());
  }


  void enqueue_error (io_t *io) noexcept
  {
    error_queue.push(io);
  }
};
using service_ptr = std::shared_ptr<service_t>;


struct worker_t //{{{1
{
  service_ptr service;

#if __sal_os_windows
  using completed_list = std::array<RIORESULT, 2048>;
#else
  using completed_list = std::array<int, 2048>;
#endif

  completed_list completed{};
  typename completed_list::iterator
    first_completed = completed.begin(),
    last_completed = completed.begin();

  static constexpr size_t min_results_per_poll = 1;
  const size_t max_results_per_poll;


  worker_t () = delete;
  worker_t (const worker_t &) = delete;
  worker_t &operator= (const worker_t &) = delete;

  worker_t (worker_t &&) = default;
  worker_t &operator= (worker_t &&) = default;


  worker_t (service_ptr service, size_t max_results_per_poll) noexcept
    : service(service)
    , max_results_per_poll(
        std::clamp(
          max_results_per_poll,
          min_results_per_poll,
          completed.max_size()
        )
      )
  {}


  bool wait_for_more (const std::chrono::milliseconds &timeout,
    std::error_code &error
  ) noexcept;


  io_t *result_at (typename completed_list::const_iterator it) noexcept;


  io_t *try_get () noexcept
  {
    if (first_completed != last_completed)
    {
      return result_at(first_completed++);
    }
    return service->dequeue_error();
  }


  io_t *poll (const std::chrono::milliseconds &timeout, std::error_code &error)
    noexcept
  {
    do
    {
      if (auto io = try_get())
      {
        return io;
      }
    } while (wait_for_more(timeout, error));

    return {};
  }
};


struct async_socket_t //{{{1
{
  using handle_t = net::__bits::socket_t::handle_t;
  using message_flags_t = net::__bits::message_flags_t;

  service_ptr service;

  uintptr_t context_type{};
  void *context{};

#if __sal_os_windows
  RIO_CQ completion_queue;
  RIO_RQ request_queue;
  sal::spinlock_t request_queue_mutex{};
#endif

  std::atomic<size_t> outstanding_recv{}, outstanding_send{};

  static handle_t open (int family, int type, int protocol);

  async_socket_t (const async_socket_t &) = delete;
  async_socket_t &operator= (const async_socket_t &) = delete;

  async_socket_t (handle_t handle,
    service_ptr service,
    size_t max_outstanding_receives,
    size_t max_outstanding_sends,
    std::error_code &error
  ) noexcept;

  ~async_socket_t () noexcept;

  void start_receive_from (io_t &io,
    void *remote_endpoint,
    size_t remote_endpoint_size,
    size_t *transferred,
    message_flags_t flags
  ) noexcept;

  void start_send_to (io_t &io,
    void *remote_endpoint,
    size_t remote_endpoint_size,
    size_t *transferred,
    message_flags_t flags
  ) noexcept;
};
using async_socket_ptr = std::unique_ptr<async_socket_t>;


//}}}1


} // namespace net::async::__bits


__sal_end
