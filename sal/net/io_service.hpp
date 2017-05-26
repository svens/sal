#pragma once

/**
 * \file sal/net/io_service.hpp
 *
 * Asynchronous I/O completion service.
 */


#include <sal/config.hpp>
#include <sal/net/basic_socket.hpp>
#include <sal/net/basic_socket_acceptor.hpp>
#include <sal/net/error.hpp>
#include <deque>


__sal_begin


namespace net {


/**
 * Asynchronous networking I/O completion service.
 *
 * This class holds platform-dependent completion handler (IOCP/epoll/kqueue)
 * but it is not meant to be used directly for polling completions. Instead,
 * per-thread io_service_t::context_t does actual completion waiting and
 * resources management.
 */
class io_service_t
{
public:

  class buf_t;
  class context_t;


  /**
   * Create new I/O completion service.
   */
  io_service_t ()
    : io_service_t(throw_on_error("io_service::io_service"))
  {}


  /**
   * Create new I/O completion thread context.
   *
   * \a max_events_per_wait configures how many events are batched when
   * waiting for completions. If this number is too little, then underlying
   * syscall must be called more often (with kernel- and user-mode switch
   * overhead). On too big value and/or if each completion handling takes too
   * long, it might take too long time to get to handling specific completion.
   */
  context_t make_context (size_t max_events_per_wait = 16);


  /**
   * Associate \a socket with this io_service_t. Calling asynchronous
   * \a socket methods without associating it first will generate error.
   * On failure, set \a error
   */
  template <typename Protocol>
  void associate (basic_socket_t<Protocol> &socket, std::error_code &error)
    noexcept
  {
    associate(socket.socket_, error);
  }


  /**
   * Associate \a socket with this io_service_t. Calling asynchronous
   * \a socket methods without associating it first will generate error.
   * On failure, throw \c std::system_error
   */
  template <typename Protocol>
  void associate (basic_socket_t<Protocol> &socket)
  {
    associate(socket, throw_on_error("io_service::associate"));
  }


  /// \copydoc associate(basic_socket_t<Protocol> &, std::error_code &)
  template <typename Protocol>
  void associate (basic_socket_acceptor_t<Protocol> &socket,
    std::error_code &error) noexcept
  {
    associate(socket.socket_, error);
  }


  /// \copydoc associate(basic_socket_t<Protocol> &)
  template <typename Protocol>
  void associate (basic_socket_acceptor_t<Protocol> &socket)
  {
    associate(socket, throw_on_error("io_service::associate"));
  }


  struct impl_t;

  /// \c internal
  using impl_ptr = std::shared_ptr<impl_t>;


private:

  __bits::async_worker_t::shared_ptr impl_;

  io_service_t (std::error_code &error) noexcept
    : impl_(std::make_shared<__bits::async_worker_t>(error))
  {}

  void associate (__bits::socket_t &socket, std::error_code &error) noexcept;
};


/**
 * Asynchronous socket I/O operation handle and associated data buffer.
 * Internally it holds continuous 4kB memory area that is divided by
 * OS-specific asynchronous call related data and I/O data for send/receive.
 *
 * This class is not meant to be instantiated directly but through
 * io_service_t::context_t::make_buf(). It's lifecycle follows strict
 * ownership:
 *   - initial owner is io_service_t::context_t free-list (per-thread pool)
 *   - after allocation and before asynchronous I/O starts, application
 *     is owner and can setup send/receive data storage
 *   - after asynchronous I/O starts, it is owned by OS and/or
 *     io_service_t::context_t (which one is undefined from application
 *     perspective)
 *   - on completion it belongs to application (completion handler), which can
 *     reuse this object or let it go out of scope (in which case it
 *     automatically returns to io_service_t::context_t free-list).
 *
 * Data area for I/O is continuous but it doesn't necessarily start at head of
 * allocated area. Each buf_t allocated data area resides between [head,tail)
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
class io_service_t::buf_t
{
public:

  /**
   * Return pointer to context_t that just finished \a this asynchronous I/O.
   */
  context_t &this_context () const noexcept
  {
    return *context_;
  }


  /**
   * Return application-specific data. sal::net itself does not use this value.
   */
  uintptr_t user_data () const noexcept
  {
    return io_.user_data;
  }


  /**
   * Set application-specific data. sal::net itself does not use this value.
   */
  void user_data (uintptr_t value) noexcept
  {
    io_.user_data = value;
  }


  /**
   * Return pointer to beginning of allocated send/receive data area.
   */
  const void *head () const noexcept
  {
    return data_;
  }


  /**
   * Return pointer to end of allocated send/receive data area.
   */
  const void *tail () const noexcept
  {
    return data_ + sizeof(data_);
  }


  /**
   * Return pointer to beginning of application set send/receive data area.
   * Falls between [head(),tail())
   */
  void *data () noexcept
  {
    return io_.begin;
  }


  /**
   * \copydoc data()
   */
  void *begin () noexcept
  {
    return io_.begin;
  }


  /**
   * Set offset of send/receive data area from head().
   * \throw std::logic_error if \a offset_from_head is past tail()
   */
  void begin (size_t offset_from_head)
  {
    sal_throw_if(offset_from_head > max_size());
    io_.begin = data_ + offset_from_head;
  }


  /**
   * Return number of bytes between [head(),begin()).
   */
  size_t head_gap () const noexcept
  {
    return io_.begin - data_;
  }


  /**
   * Return number of bytes between [end(),tail())
   */
  size_t tail_gap () const noexcept
  {
    return data_ + sizeof(data_) - io_.end;
  }


  /**
   * Return pointer to end of application set send/receive data area. Falls
   * between [begin(),tail()]
   */
  const void *end () const noexcept
  {
    return io_.end;
  }


  /**
   * Return number of bytes between [begin(),end()) ie send/receive data
   * size.
   */
  size_t size () const noexcept
  {
    return io_.end - io_.begin;
  }


  /**
   * Set size for send/receive data size ie begin() + \a s = end().
   * \throw std::logic_error if \a new_size sets end() past tail().
   */
  void resize (size_t new_size)
  {
    sal_throw_if(io_.begin + new_size > tail());
    io_.end = io_.begin + new_size;
  }


  /**
   * Return compile time reserved data area size (in bytes).
   */
  static constexpr size_t max_size () noexcept
  {
    return sizeof(data_);
  }


  /**
   * Set begin() == head() and end() == tail()
   */
  void reset () noexcept
  {
    io_.begin = data_;
    io_.end = data_ + sizeof(data_);
  }


  /**
   * Start asynchronous \a IO request passing \a args to worker method.
   * \internal Do not use directly but socket's asynchronous API
   */
  template <typename IO, typename... Args>
  void start (Args &&...args) noexcept
  {
    static_assert(sizeof(IO) <= sizeof(io_) + sizeof(io_data_));
    static_assert(std::is_trivially_destructible<IO>::value);
    io_.request_id = IO::type_id();
    reinterpret_cast<IO *>(this)->start(std::forward<Args>(args)...);
  }


  /**
   * After completion, try to cast to specified \a IO result type. On success,
   * returns pointer to requested result data, \c nullptr otherwise.
   * \internal Do not use directly but socket's asynchronous API
   */
  template <typename IO>
  IO *result () noexcept
  {
    if (io_.request_id == IO::type_id())
    {
      return reinterpret_cast<IO *>(this);
    }
    return nullptr;
  }


private:

  // Ugly hack (reinterpret_cast)
  //  - io_ must be 1st member (for winsock OVERLAPPED == this)
  //  - io_data_ must follow immediately io_, specific I/O operation
  //    parameters are kept there
  __bits::io_buf_t io_{};
  char io_data_[160];

  context_t * const owner_;
  context_t *context_{};

  char data_[4096
    - sizeof(decltype(io_))
    - sizeof(decltype(io_data_))
    - sizeof(decltype(owner_))
    - sizeof(decltype(context_))
  ];


  static constexpr void static_check ()
  {
    static_assert(sizeof(buf_t) == 4096);
    static_assert(sizeof(data_) > 1500);
  }


  buf_t (context_t *owner) noexcept
    : owner_(owner)
  {}


  friend class context_t;
};

/// Strict ownership pointer to asynchronous I/O operation handle
using io_buf_ptr = std::unique_ptr<io_service_t::buf_t, void(*)(io_service_t::buf_t *)>;


/**
 * Per I/O thread representative of io_service_t. It also maintains per-thread
 * resources (io_buf_ptr pool, etc). Each instance of context_t maintains own
 * operations' and completions' queues.
 *
 * To wait for completions, call poll() repeatedly that returns next
 * completion from queue. If queue is empty, it uses io_service_t to get batch
 * of next completions that'll be returned one by one to application.
 *
 * This class is not meant to be instantiated directly but through
 * io_service_t::make_context(). Instances must be kept alive until all of
 * buffers allocated using make_buf() are finished and returned to owner
 * thread's context_t pool.
 */
class io_service_t::context_t
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
  io_buf_ptr make_buf ()
  {
    io_buf_ptr io_buf{alloc_buf(), &context_t::free_buf};
    io_buf->reset();
    io_buf->context_ = this;
    return io_buf;
  }


private:

  __bits::async_worker_t::shared_ptr worker_;
  std::deque<std::unique_ptr<char, void(*)(char *)>> pool_{};
  __bits::io_buf_t::free_list free_{};


  context_t (__bits::async_worker_t::shared_ptr worker,
      size_t max_events_per_wait)
    : worker_(worker)
  {
    (void)max_events_per_wait;
  }


  //
  // alloc_buf / free_buf: see "Ugly hack" in buf_t
  //

  void extend_pool ();


  buf_t *alloc_buf ()
  {
    auto io_buf = free_.try_pop();
    if (!io_buf)
    {
      extend_pool();
      io_buf = free_.try_pop();
    }
    return reinterpret_cast<buf_t *>(io_buf);
  }


  static void free_buf (buf_t *io_buf) noexcept
  {
    io_buf->owner_->free_.push(reinterpret_cast<__bits::io_buf_t *>(io_buf));
  }


  friend class io_service_t;
};


inline io_service_t::context_t io_service_t::make_context (
  size_t max_events_per_wait)
{
  return context_t{impl_, max_events_per_wait};
}


} // namespace net


__sal_end
