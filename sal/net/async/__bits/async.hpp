#pragma once

#include <sal/config.hpp>
#include <sal/assert.hpp>
#include <sal/intrusive_mpsc_queue.hpp>
#include <sal/intrusive_queue.hpp>
#include <sal/net/__bits/socket.hpp>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>


__sal_begin


namespace net::async::__bits {


struct handler_t;


struct io_base_t //{{{1
{
#if __sal_os_windows
  OVERLAPPED overlapped{};
#endif

  handler_t *current_owner{};
  uintptr_t context_type{};
  void *context{};

  uintptr_t result_type{};
  uint8_t result_data[160];
  std::error_code status{};

  uint8_t *begin{}, *end{};

  union
  {
    intrusive_mpsc_queue_hook_t<io_base_t> free{};
    intrusive_queue_hook_t<io_base_t> completed;
  };
  using free_list_t = intrusive_mpsc_queue_t<&io_base_t::free>;
  using completed_queue_t = intrusive_mpsc_queue_t<&io_base_t::completed>;

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


struct service_t //{{{1
{
  std::mutex io_pool_mutex{};
  std::deque<std::vector<io_t>> io_pool{};
  io_t::free_list_t free_list{};
  size_t io_pool_size{};


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
    auto io = sal_check_ptr(alloc_io());
    io->current_owner = nullptr;
    io->context_type = context_type;
    io->context = context;
    io->reset();
    return io;
  }
};
using service_ptr = std::shared_ptr<service_t>;


struct worker_t //{{{1
{
};


struct handler_t //{{{1
{
  service_ptr service;

  uintptr_t context_type{};
  void *context;
};
using handler_ptr = std::unique_ptr<handler_t>;


struct io_deleter_t //{{{1
{
  void operator() (io_t *io) noexcept
  {
    io->free_list.push(io);
  }
};
using io_ptr = std::unique_ptr<io_t, io_deleter_t>;


//}}}1


} // namespace net::async::__bits


__sal_end
