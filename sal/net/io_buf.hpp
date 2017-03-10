#pragma once

/**
 * \file sal/net/io_buf.hpp
 */


#include <sal/config.hpp>
#include <sal/net/__bits/io_service.hpp>
#include <sal/net/fwd.hpp>
#include <sal/assert.hpp>
#include <sal/intrusive_queue.hpp>
#include <memory>


__sal_begin


namespace net {


/**
 * Asynchronous socket operation I/O buffer.
 *
 * \pre
 * head                                   tail
 * v                                         v
 * .....ooooooooooooooooooooooooooooooo......
 *      ^                              ^
 *      begin                        end
 * \endpre
 */
class io_buf_t
  : public __bits::io_buf_t
{
  using buf = __bits::io_buf_t;

public:

  io_buf_t (const io_buf_t &) = delete;
  io_buf_t &operator= (const io_buf_t &) = delete;


  io_context_t &this_context () const noexcept
  {
    return *reinterpret_cast<io_context_t *>(buf::context);
  }


  uintptr_t user_data () const noexcept
  {
    return buf::user_data;
  }


  void user_data (uintptr_t value) noexcept
  {
    buf::user_data = value;
  }


  const void *head () const noexcept
  {
    return data_;
  }


  const void *tail () const noexcept
  {
    return data_ + sizeof(data_);
  }


  void *data () noexcept
  {
    return buf::begin;
  }


  void *begin () noexcept
  {
    return buf::begin;
  }


  void begin (size_t offset_from_head)
  {
    sal_assert(offset_from_head < sizeof(data_));
    buf::begin = data_ + offset_from_head;
  }


  size_t head_gap () const noexcept
  {
    return buf::begin - data_;
  }


  size_t tail_gap () const noexcept
  {
    return data_ + sizeof(data_) - buf::end;
  }


  const void *end () const noexcept
  {
    return buf::end;
  }


  size_t size () const noexcept
  {
    return buf::end - buf::begin;
  }


  void resize (size_t s)
  {
    sal_assert(buf::begin + s <= data_ + sizeof(data_));
    buf::end = buf::begin + s;
  }


  static constexpr size_t max_size () noexcept
  {
    return sizeof(data_);
  }


  void reset () noexcept
  {
    buf::begin = data_;
    buf::end = data_ + max_size();
  }


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


using io_buf_ptr = std::unique_ptr<io_buf_t, void(*)(io_buf_t*)>;


} // namespace net


__sal_end
