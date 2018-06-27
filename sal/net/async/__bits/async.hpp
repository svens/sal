#pragma once

#include <sal/config.hpp>
#include <sal/intrusive_mpsc_queue.hpp>
#include <sal/net/__bits/socket.hpp>
#include <algorithm>
#include <atomic>
#include <deque>
#include <memory>


__sal_begin


namespace net::async::__bits {


struct io_t;
struct io_block_t;
struct service_t;
struct context_t;
struct socket_t;


//
// io_t
//
struct io_base_t
{
  io_block_t &io_block;

  void *user_data{};
  uintptr_t op_id{};
  uint8_t op_data[1];
  std::error_code error{};

  uint8_t *begin{}, *end{};

  union
  {
    intrusive_mpsc_queue_hook_t<io_base_t> free{};
  };
  using free_list_t = intrusive_mpsc_queue_t<&io_base_t::free>;


  io_base_t (io_block_t &io_block) noexcept
    : io_block(io_block)
  { }
};


struct io_t
  : public io_base_t
{
  static constexpr size_t data_size = 2048 - sizeof(io_base_t);
  uint8_t data[data_size];


  io_t (io_block_t &io_block) noexcept
    : io_base_t(io_block)
  { }
};
static_assert(sizeof(io_t) == 2048);
static_assert(io_t::data_size > 1500, "io_t::data_size less than MTU size");


struct io_block_t
{
  io_t::free_list_t &free;
  std::unique_ptr<char[]> data;

#if __sal_os_windows
  RIO_BUFFERID buffer_id;
#endif

  io_block_t (size_t size, io_t::free_list_t &free);
  ~io_block_t () noexcept;
};


struct io_deleter_t
{
  void operator() (io_t *io) noexcept
  {
    io->io_block.free.push(io);
  }
};
using io_ptr = std::unique_ptr<io_t, io_deleter_t>;


//
// service_t
//
struct service_t
{
#if __sal_os_windows
  HANDLE iocp;
  OVERLAPPED overlapped;
  RIO_CQ completed;
#endif

  static constexpr size_t max_events_per_poll = 256;

  std::deque<io_block_t> pool{};
  io_t::free_list_t free{};
  size_t io_pool_size{0};


  service_t (size_t completion_queue_size);
  ~service_t () noexcept;


  io_t *make_io (void *user_data = nullptr)
  {
    auto io = static_cast<io_t *>(free.try_pop());
    if (!io)
    {
      // extend pool (start with 512, 2x every next alloc)
      size_t block_size = 512 * sizeof(io_t) * (1ULL << pool.size());
      pool.emplace_back(block_size, free);
      io_pool_size += block_size;
      io = static_cast<io_t *>(free.try_pop());
    }
    io->begin = io->data;
    io->end = io->data + sizeof(io->data);
    io->user_data = user_data;
    return io;
  }
};
using service_ptr = std::shared_ptr<service_t>;


//
// context_t
//
struct context_t
{
  service_ptr service;
  size_t max_events_per_poll;


  context_t () = delete;
  context_t (const context_t &) = delete;
  context_t &operator= (const context_t &) = delete;

  context_t (context_t &&) = default;
  context_t &operator= (context_t &&) = default;


  context_t (service_ptr service, size_t max_events_per_poll)
    : service(service)
    , max_events_per_poll(
        std::clamp(max_events_per_poll,
          size_t{1}, service_t::max_events_per_poll
        )
      )
  { }
};


//
// socket_t
//
struct socket_t
  : protected net::__bits::socket_t
{
  socket_t (handle_t handle) noexcept
    : net::__bits::socket_t(handle)
  { }
};
using socket_ptr = std::unique_ptr<socket_t>;


} // namespace net::async::__bits


__sal_end
