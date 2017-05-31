#pragma once

/**
 * \file sal/net/async_service.hpp
 *
 * Asynchronous I/O completion service.
 */


#include <sal/config.hpp>
#include <sal/net/__bits/socket.hpp>
#include <sal/net/error.hpp>


__sal_begin


namespace net {


/**
 * Asynchronous networking I/O completion service.
 *
 * This class holds platform-dependent completion handler (IOCP/epoll/kqueue)
 * but it is not meant to be used directly for polling completions. Instead,
 * per-thread async_service_t::context_t does actual completion waiting and
 * resources management.
 */
class async_service_t
{
public:

  class io_t;
  class context_t;


  /**
   * Create new I/O completion thread context.
   *
   * \a max_events_per_poll configures how many events are batched when
   * waiting for completions. If this number is too little, then underlying
   * syscall must be called more often (with kernel- and user-mode switch
   * overhead). On too big value and/or if each completion handling takes too
   * long, it might take too long time to get to handling specific completion.
   */
  context_t make_context (size_t max_events_per_poll = 16);


private:

  __bits::async_service_ptr impl_ = std::make_shared<__bits::async_service_t>(
    throw_on_error("async_service")
  );

  friend class socket_base_t;
};


/**
 * Asynchronous socket I/O operation handle and associated data buffer.
 * Internally it holds continuous 4kB memory area that is divided by
 * OS-specific asynchronous call related data and I/O data for send/receive.
 *
 * This class is not meant to be instantiated directly but through
 * async_service_t::context_t::make_io(). It's lifecycle follows strict
 * ownership:
 *   - initial owner is async_service_t::context_t free-list (per-thread pool)
 *   - after allocation and before asynchronous I/O starts, application
 *     is owner and can setup send/receive data storage
 *   - after asynchronous I/O starts, it is owned by OS and/or
 *     async_service_t::context_t (which one is undefined from application
 *     perspective)
 *   - on completion it belongs to application (completion handler), which can
 *     reuse this object or let it go out of scope (in which case it
 *     automatically returns to async_service_t::context_t free-list).
 *
 * Data area for I/O is continuous but it doesn't necessarily start at head of
 * allocated area. Each io_t allocated data area resides between [head,tail)
 * but when launching asynchronous send/receive operations, actually used data
 * is range [begin,end):
 * ~~~
 *                   size
 * head   _____________^_______________    tail
 * v     /                             \      v
 * ......ooooooooooooooooooooooooooooooo......
 * |     ^                              ^     |
 * |     begin/data                   end     |
 * |__ __|                              |__ __|
 *    V                                    V
 * head_gap                             tail_gap
 * ~~~
 *
 * This allows application to build packet header into [head,begin) and/or
 * trailer into [end,tail)
 */
class async_service_t::io_t
  : public __bits::async_io_t
{
public:

  /**
   * Return pointer to context_t that just finished \a this asynchronous I/O.
   */
  context_t &this_context () const noexcept
  {
    return *reinterpret_cast<context_t *>(async_io_t::context);
  }


  /**
   * Return application-specific data. sal::net itself does not use this value.
   */
  uintptr_t user_data () const noexcept
  {
    return async_io_t::user_data;
  }


  /**
   * Set application-specific data. sal::net itself does not use this value.
   */
  void user_data (uintptr_t value) noexcept
  {
    async_io_t::user_data = value;
  }


  /**
   * Return pointer to beginning of allocated send/receive data area.
   */
  const void *head () const noexcept
  {
    return *async_io_t::data;
  }


  /**
   * Return pointer to end of allocated send/receive data area.
   */
  const void *tail () const noexcept
  {
    return *async_io_t::data + sizeof(*async_io_t::data);
  }


  /**
   * Return pointer to beginning of application set send/receive data area.
   * Falls between [head(),tail())
   */
  void *data () noexcept
  {
    return async_io_t::begin;
  }


  /**
   * \copydoc data()
   */
  void *begin () noexcept
  {
    return async_io_t::begin;
  }


  /**
   * Set offset of send/receive data area from head().
   * \throw std::logic_error if \a offset_from_head is past tail()
   */
  void begin (size_t offset_from_head)
  {
    sal_throw_if(offset_from_head > max_size());
    async_io_t::begin = *async_io_t::data + offset_from_head;
  }


  /**
   * Return pointer to end of application set send/receive data area. Falls
   * between [begin(),tail()]
   */
  const void *end () const noexcept
  {
    return async_io_t::end;
  }


  /**
   * Return number of bytes between [head(),begin()).
   */
  size_t head_gap () const noexcept
  {
    return async_io_t::begin - *async_io_t::data;
  }


  /**
   * Return number of bytes between [end(),tail())
   */
  size_t tail_gap () const noexcept
  {
    return *async_io_t::data + sizeof(*async_io_t::data) - async_io_t::end;
  }


  /**
   * Return number of bytes between [begin(),end()) ie send/receive data
   * size.
   */
  size_t size () const noexcept
  {
    return async_io_t::end - async_io_t::begin;
  }


  /**
   * Set size for send/receive data size ie begin() + \a s = end().
   * \throw std::logic_error if \a new_size sets end() past tail().
   */
  void resize (size_t new_size)
  {
    sal_throw_if(async_io_t::begin + new_size > tail());
    async_io_t::end = async_io_t::begin + new_size;
  }


  /**
   * Return compile time reserved data area size (in bytes).
   */
  static constexpr size_t max_size () noexcept
  {
    return sizeof(*async_io_t::data);
  }


  /**
   * Set begin() == head() and end() == tail()
   */
  void reset () noexcept
  {
    async_io_t::begin = *async_io_t::data;
    async_io_t::end = *async_io_t::data + sizeof(*async_io_t::data);
  }
};

/// Strict ownership pointer to asynchronous I/O operation handle
using io_ptr = std::unique_ptr<async_service_t::io_t, void(*)(void *)>;


/**
 * Per I/O thread representative of async_service_t. It also maintains
 * per-thread resources (io_ptr pool, etc). Each instance of context_t
 * maintains own operations' and completions' queues.
 *
 * To wait for completions, call poll() repeatedly that returns next
 * completion from queue. If queue is empty, it uses async_service_t to get
 * batch of next completions that'll be returned one by one to application.
 *
 * This class is not meant to be instantiated directly but through
 * async_service_t::make_context(). Instances must be kept alive until all of
 * buffers allocated using make_io() are finished and returned to owner
 * thread's context_t pool.
 */
class async_service_t::context_t
  : protected __bits::async_context_t
{
public:

  /// Move ctor
  context_t (context_t &&) = default;
  /// Move assign
  context_t &operator= (context_t &&) = default;

  context_t (const context_t &) = delete;
  context_t &operator= (const context_t &) = delete;


  /**
   * Allocate I/O operation handle for asynchronous operation. It first tries
   * to return handle from internal pool of free handles. On exhaustion, next
   * batch of handles are allocated and one of them is returned. On allocation
   * failure, internal ```std::dequeue::emplace_back()``` throws an exception.
   */
  io_ptr make_io ()
  {
    return io_ptr{
      static_cast<io_t *>(async_context_t::new_io()),
      &async_context_t::release_io
    };
  }


  /**
   * Return completed I/O operation handle for asynchronous operation or
   * nullptr if none. This method does not wait for actual completions but
   * returns already completed from queue.
   */
  io_ptr try_get () noexcept
  {
    return io_ptr{
      static_cast<io_t *>(async_context_t::try_get()),
      &async_context_t::release_io
    };
  }


  /**
   * Return completed I/O operation handle for asynchronous operation. If none
   * is immediately available, this methods wait for more completions until
   * \a timeout has passed. If still no completions, \c nullptr is returned.
   *
   * On waiting failure, \a error is set and nullptr is returned.
   */
  template <typename Rep, typename Period>
  io_ptr poll (const std::chrono::duration<Rep, Period> &timeout,
    std::error_code &error) noexcept
  {
    return io_ptr{
      static_cast<io_t *>(async_context_t::poll(timeout, error)),
      &async_context_t::release_io
    };
  }


  /**
   * Return completed I/O operation handle for asynchronous operation. If none
   * is immediately available, this methods wait for more completions until
   * \a timeout has passed. If still no completions, \c nullptr is returned.
   *
   * \throws std::system_error if waiting failed
   */
  template <typename Rep, typename Period>
  io_ptr poll (const std::chrono::duration<Rep, Period> &timeout)
  {
    return poll(timeout, throw_on_error("async_service::poll"));
  }


  /**
   * Return completed I/O operation handle for asynchronous operation. If none
   * is immediately available, this methods waits indefinitely until more
   * completions are available.
   *
   * On waiting failure, \a error is set and nullptr is returned.
   */
  io_ptr poll (std::error_code &error) noexcept
  {
    return poll((std::chrono::milliseconds::max)(), error);
  }


  /**
   * Return completed I/O operation handle for asynchronous operation. If none
   * is immediately available, this methods waits indefinitely until more
   * completions are available.
   *
   * \throws std::system_error if waiting failed
   */
  io_ptr poll ()
  {
    return poll((std::chrono::milliseconds::max)());
  }


  /**
   * Return completed I/O operation handle for asynchronous operation. If none
   * is immediately available, returns \c nullptr.
   *
   * On poll failure, \a error is set and nullptr is returned.
   */
  io_ptr try_poll (std::error_code &error) noexcept
  {
    return poll(std::chrono::milliseconds(0), error);
  }


  /**
   * Return completed I/O operation handle for asynchronous operation. If none
   * is immediately available, returns \c nullptr.
   *
   * \throws std::system_error if polling failed
   */
  io_ptr try_poll ()
  {
    return poll(std::chrono::milliseconds(0));
  }


  /**
   * Release already completed asynchronous I/O operations.
   * \returns number of completions released
   */
  size_t reclaim () noexcept
  {
    auto count = 0U;
    while (auto *io = async_context_t::try_get())
    {
      async_context_t::release_io(io);
      ++count;
    }
    return count;
  }


private:

  context_t (__bits::async_service_ptr service, size_t max_events_per_poll)
    : async_context_t(service, max_events_per_poll)
  {}

  friend class async_service_t;
};


inline async_service_t::context_t async_service_t::make_context (
  size_t max_events_per_poll)
{
  return context_t{impl_, max_events_per_poll};
}


} // namespace net


__sal_end
