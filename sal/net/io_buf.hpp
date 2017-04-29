#pragma once

/**
 * \file sal/net/io_buf.hpp
 *
 * Generic asynchronous operation buffer.
 */


#include <sal/config.hpp>
#include <sal/net/__bits/async_socket.hpp>
#include <sal/net/fwd.hpp>
#include <sal/assert.hpp>
#include <sal/intrusive_queue.hpp>
#include <memory>


__sal_begin


namespace net {


/**
 * Asynchronous socket operation handle and associated I/O data buffer.
 * Internally it holds continuous 4kB memory area that is divided by
 * OS-specific asynchronous call related data and I/O data for send/receive.
 *
 * This class is not meant to be instantiated directly but through
 * io_context_t::make_buf(). It's lifecycle follows strict ownership:
 *   - initial owner is io_context_t free-list (per-thread pool)
 *   - after allocation and before asynchronous operation start, application
 *     is owner and can setup send/receive data storage
 *   - after asynchronous operation start, it is owned by OS and/or
 *     io_context_t operation queue
 *   - on completion it belongs to application (completion handler), which can
 *     reuse this object or let it go out of scope (in which case it
 *     automatically returns to io_context_t free-list).
 *
 * Data area for I/O is continuous but it doesn't necessarily start at head of
 * allocated area. Each io_buf_t allocated data area resides between
 * [head,tail) but when launching asynchronous send/receive operations,
 * actually used data is range [begin,end):
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
class io_buf_t
  : public __bits::io_buf_t
{
  using buf = __bits::io_buf_t;

public:

  io_buf_t (const io_buf_t &) = delete;
  io_buf_t &operator= (const io_buf_t &) = delete;


  /**
   * Return pointer to io_context_t that just finished \a this asynchronous
   * operation.
   */
  io_context_t &this_context () const noexcept
  {
    return *reinterpret_cast<io_context_t *>(buf::context);
  }


  /**
   * Return application-specific data. Networking library itself does not use
   * this value.
   */
  uintptr_t user_data () const noexcept
  {
    return buf::user_data;
  }


  /**
   * Set application-specific data. Networking library itself does not use
   * this value.
   */
  void user_data (uintptr_t value) noexcept
  {
    buf::user_data = value;
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
    return buf::begin;
  }


  /**
   * \copydoc data()
   */
  void *begin () noexcept
  {
    return buf::begin;
  }


  /**
   * Set offset of send/receive data area from head(). In debug build, this
   * method checks that it falls between [head(),tail()).
   */
  void begin (size_t offset_from_head)
  {
    sal_assert(offset_from_head < sizeof(data_));
    buf::begin = data_ + offset_from_head;
  }


  /**
   * Return number of bytes between [head(),begin()).
   */
  size_t head_gap () const noexcept
  {
    return buf::begin - data_;
  }


  /**
   * Return number of bytes between [end(),tail())
   */
  size_t tail_gap () const noexcept
  {
    return data_ + sizeof(data_) - buf::end;
  }


  /**
   * Return pointer to end of application set send/receive data area. Falls
   * between [begin(),tail()]
   */
  const void *end () const noexcept
  {
    return buf::end;
  }


  /**
   * Return number of bytes between [begin(),end()) ie send/receive data
   * size.
   */
  size_t size () const noexcept
  {
    return buf::end - buf::begin;
  }


  /**
   * Set size for send/receive data size ie begin() + \a s = end(). In debug
   * build, this method checks that begin() + \a s <= tail()
   */
  void resize (size_t s)
  {
    sal_assert(buf::begin + s <= data_ + sizeof(data_));
    buf::end = buf::begin + s;
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
    buf::begin = data_;
    buf::end = data_ + max_size();
  }


  /**
   * Start asynchronous \a Request passing \a args to worker method.
   * \internal Do not use directly but socket's asynchronous API
   */
  template <typename Request, typename... Args>
  void start (Args &&...args) noexcept
  {
    static_assert(sizeof(Request) <= sizeof(buf) + sizeof(request_data_),
      "sizeof(Request) exceeds request data buffer"
    );
    static_assert(std::is_trivially_destructible<Request>::value,
      "expected Request to be trivially destructible"
    );
    buf::request_id = Request::type_id();
    reinterpret_cast<Request *>(this)->start(std::forward<Args>(args)...);
  }


  /**
   * After completion, try to cast to specified \a Result type. On success,
   * returns pointer to requested result data, \c nullptr otherwise.
   * \internal Do not use directly but socket's asynchronous API
   */
  template <typename Result>
  Result *result () noexcept
  {
    if (buf::request_id == Result::type_id())
    {
      return reinterpret_cast<Result *>(this);
    }
    return nullptr;
  }


private:

  char request_data_[160];
  io_context_t * const owner_;

  mpsc_sync_t::intrusive_queue_hook_t free_{};
  using free_list = intrusive_queue_t<
    io_buf_t, mpsc_sync_t, &io_buf_t::free_
  >;

  static constexpr size_t members_size = sizeof(buf)
    + sizeof(decltype(request_data_))
    + sizeof(decltype(owner_))
    + sizeof(decltype(free_));

  char data_[4096 - members_size];


  io_buf_t (io_context_t *owner) noexcept
    : owner_(owner)
  {
    (void)free_;
  }


  static constexpr void static_check ()
  {
    static_assert(sizeof(io_buf_t) == 4096,
      "expected sizeof(io_buf_t) == 4096B"
    );
  }


  friend class io_context_t;
};


/**
 * String owner pointer to asyncronous operation handle.
 */
using io_buf_ptr = std::unique_ptr<io_buf_t, void(*)(io_buf_t*)>;


} // namespace net


__sal_end
