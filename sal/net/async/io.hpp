#pragma once

/**
 * \file sal/net/async/io.hpp
 * Asynchronous network I/O operation
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/async/__bits/async.hpp>
#include <sal/assert.hpp>
#include <sal/type_id.hpp>


__sal_begin


namespace net::async {


/**
 * Asynchronous socket I/O operation handle and associated data for I/O (2kB).
 *
 * This class is not meant to be instantiated directly but through
 * service_t::make_io(). It's lifecycle follows strict ownership:
 *   - initial owner is service_t (internal free-list)
 *   - after allocation and before asynchronous I/O starts, application
 *     is owner and can setup I/O data storage
 *   - after asynchronous I/O starts, it is owned by OS or service_t (which
 *     one is undefined from application perspective)
 *   - on completion it belongs to application (completion handler), which can
 *     reuse this object or let it go out of scope (in which case it
 *     automatically returns to service_t free-list).
 *
 * Data area for I/O is continuous but it doesn't necessarily start at head of
 * allocated area. Each io_t allocated data area resides between
 * [head(),tail()) but when launching asynchronous send/receive operations,
 * actually used data is range [begin(),end()):
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
 * This allows application to build packet header into [head(),begin()) and/or
 * trailer into [end(),tail()).
 */
class io_t
{
public:

  io_t () = delete;
  io_t (const io_t &) = delete;
  io_t &operator= (const io_t &) = delete;
  io_t (io_t &&) = delete;
  io_t &operator= (io_t &&) = delete;


  /**
   * io_ptr deleter
   * \internal
   */
  struct deleter_t
  {
    /**
     * Release io_t to owner (service_t) free list for later reuse.
     */
    void operator() (io_t *io) noexcept
    {
      io->impl_.owner.free_list.push(&io->impl_);
    }
  };


  /**
   * Set begin() == head() and end() == tail()
   */
  void reset () noexcept
  {
    impl_.begin = impl_.data;
    impl_.end = impl_.data + max_size();
  }


  /**
   * Set application-specific I/O context. Internally this field is not used
   * by library. Application can use it to store additional data related to
   * this specific asynchronous I/O. On allocation, this field is nullied.
   */
  template <typename Context>
  void context (Context *context) noexcept
  {
    impl_.context = context;
    impl_.context_type = type_v<Context>;
  }


  /**
   * Get application-specific I/O context only if it has expected Context type.
   * \see context(Context *)
   */
  template <typename Context>
  Context *context () const noexcept
  {
    if (impl_.context_type == type_v<Context>)
    {
      return static_cast<Context *>(impl_.context);
    }
    return nullptr;
  }


  /**
   * Return application-specific socket context. Internally this field is not
   * used by library. Application can store additional data related to socket.
   * Use this method to query socket's context on it's asynchonous I/O
   * completion.
   *
   * \note This getter returns pointer to socket context only if it has
   * expected Context type.
   */
  template <typename Context>
  Context *socket_context () const
  {
    auto &current_owner = *sal_check_ptr(impl_.current_owner);
    if (current_owner.context_type == type_v<Context>)
    {
      return static_cast<Context *>(current_owner.context);
    }
    return nullptr;
  }


  /**
   * Return pointer to beginning of allocated send/receive data area.
   */
  const std::byte *head () const noexcept
  {
    return impl_.data;
  }


  /**
   * Return pointer to end of allocated send/receive data area.
   */
  const std::byte *tail () const noexcept
  {
    return impl_.data + max_size();
  }


  /**
   * Return pointer to beginning of application set send/receive data area.
   * Falls between [head(),tail())
   */
  std::byte *data () noexcept
  {
    return impl_.begin;
  }


  /**
   * \copydoc data()
   */
  std::byte *begin () noexcept
  {
    return impl_.begin;
  }


  /**
   * Return pointer to end of application set send/receive data area. Falls
   * between [begin(),tail()]
   */
  const std::byte *end () const noexcept
  {
    return impl_.end;
  };


  /**
   * Set offset of send/receive data area from head().
   * \throw std::logic_error if \a offset_from_head is past tail()
   */
  void head_gap (size_t offset_from_head)
  {
    sal_assert(offset_from_head <= max_size());
    impl_.begin = impl_.data + offset_from_head;
  }


  /**
   * Return number of bytes between [head(),begin()).
   */
  size_t head_gap () const noexcept
  {
    return impl_.begin - head();
  }


  /**
   * Set offset for end of send/receive data area from tail().
   * \throw std::logic_error if \a offset_from_tail is past head()
   */
  void tail_gap (size_t offset_from_tail)
  {
    sal_assert(offset_from_tail <= max_size());
    impl_.end = tail() - offset_from_tail;
  }


  /**
   * Return number of bytes between [end(),tail())
   */
  size_t tail_gap () const noexcept
  {
    return tail() - impl_.end;
  }


  /**
   * Return number of bytes between [begin(),end()) ie send/receive data
   * size.
   */
  size_t size () const noexcept
  {
    return impl_.end - impl_.begin;
  }


  /**
   * Set size for send/receive data size ie begin() + \a s = end().
   * \throw std::logic_error if \a new_size sets end() past tail().
   */
  void resize (size_t new_size)
  {
    sal_assert(impl_.begin + new_size <= tail());
    impl_.end = impl_.begin + new_size;
  }


  /**
   * Return data area size (in bytes).
   */
  static constexpr size_t max_size () noexcept
  {
    return sizeof(impl_.data);
  }


  /**
   * On completed I/O operation, return pointer to result data. It returns
   * valid pointer only if this object represents expected operation
   * \a Result and \c nullptr otherwise.
   *
   * If asynchonous I/O finished with failure, error code is returned in
   * \a error. In this case returned pointer is valid but it's fields' values
   * are undefined.
   */
  template <typename Result>
  const Result *get_if (std::error_code &error) const noexcept
  {
    if (impl_.op == type_v<Result>)
    {
      error = impl_.status;
      return reinterpret_cast<const Result *>(impl_.result);
    }
    return nullptr;
  }


  /**
   * On completed I/O operation, return pointer to result data. It returns
   * valid pointer only if this object represents expected operation
   * \a Result and \c nullptr otherwise.
   *
   * \throws std::system_error if I/O operation finished with error.
   */
  template <typename Result>
  const Result *get_if () const
  {
    return get_if<Result>(throw_on_error("io::get_if"));
  }


  /**
   * \see get_if(std::error_code &)
   */
  template <typename Result>
  Result *get_if (std::error_code &error) noexcept
  {
    if (impl_.op == type_v<Result>)
    {
      error = impl_.status;
      return reinterpret_cast<Result *>(impl_.result);
    }
    return nullptr;
  }


  /**
   * \see get_if()
   */
  template <typename Result>
  Result *get_if ()
  {
    return get_if<Result>(throw_on_error("io::get_if"));
  }


private:

  __bits::io_t impl_;


  template <typename Result>
  Result *prepare () noexcept
  {
    static_assert(std::is_trivially_destructible_v<Result>);
    static_assert(sizeof(Result) <= sizeof(impl_.result));
    impl_.op = type_v<Result>;
    return reinterpret_cast<Result *>(impl_.result);
  }


  friend class service_t;
  template <typename Protocol> friend class net::basic_datagram_socket_t;
  template <typename Protocol> friend class net::basic_stream_socket_t;
  template <typename Protocol> friend class net::basic_socket_acceptor_t;
};


/**
 * Unique pointer to asynchronous I/O. On release, I/O block is returned to
 * service's pool of I/O blocks for reuse.
 */
using io_ptr = std::unique_ptr<io_t, io_t::deleter_t>;


} // namespace net::async


__sal_end
