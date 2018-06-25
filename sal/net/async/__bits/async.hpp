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
struct service_t;
struct context_t;
struct socket_t;


//
// io_t
//
struct io_base_t
{
#if __sal_os_windows
  RIO_BUF buf;
#endif

  context_t * const owner, *this_context{};

  union
  {
    intrusive_mpsc_queue_hook_t<io_base_t> free{};
  };
  using free_list = intrusive_mpsc_queue_t<&io_base_t::free>;

  uintptr_t user_data_type{};
  void *user_data{};


  io_base_t (context_t *owner) noexcept
    : owner(owner)
  { }
};


struct io_t
  : public io_base_t
{
  static constexpr size_t data_size = 2048 - sizeof(io_base_t);
  char data[data_size];


  io_t (context_t *owner) noexcept
    : io_base_t(owner)
  { }
};
static_assert(sizeof(io_t) == 2048);
static_assert(io_t::data_size > 1500, "io_t::data_size less than MTU size");


//
// service_t
//
struct service_t
{
#if __sal_os_windows
  HANDLE iocp;
#endif

  static constexpr size_t max_events_per_poll = 256;
  size_t io_pool_size{0};

  service_t (std::error_code &error) noexcept;
  ~service_t () noexcept;
};
using service_ptr = std::shared_ptr<service_t>;


//
// context_t
//
struct context_t
{
  service_ptr service;
  size_t max_events_per_poll;

  std::deque<std::unique_ptr<char[]>> pool{};
  io_t::free_list free{};


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


  io_t *make_io (uintptr_t type = 0, void *user_data = nullptr);
};


struct io_free_t
{
  void operator() (io_t *io) noexcept
  {
    io->owner->free.push(io);
  }
};
using io_ptr = std::unique_ptr<io_t, io_free_t>;


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
