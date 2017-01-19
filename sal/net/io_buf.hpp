#pragma once

/**
 * \file sal/net/io_buf.hpp
 */


#include <sal/config.hpp>
#include <sal/net/__bits/async.hpp>
#include <sal/net/fwd.hpp>
#include <sal/assert.hpp>
#include <sal/intrusive_queue.hpp>
#include <cstring>
#include <memory>


__sal_begin


namespace net {


namespace __bits {


constexpr size_t round_next_4 (size_t s)
{
  return (s + 3) & ~3;
}


// for msvc poor implementation of constexpr
union hooks_t
{
  mpsc_sync_t::intrusive_queue_hook_t free;
  no_sync_t::intrusive_queue_hook_t completed;
};

constexpr size_t max_hook_size = sizeof(hooks_t);



} // namespace __bits



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
  : protected __bits::io_buf_aux_t
{
public:

  io_buf_t (const io_buf_t &) = delete;
  io_buf_t &operator= (const io_buf_t &) = delete;


  io_context_t *this_context () const noexcept
  {
    return this_context_;
  }


  uintptr_t request_data () const noexcept
  {
    return request_data_;
  }


  void request_data (uintptr_t data) noexcept
  {
    request_data_ = data;
  }


  uintptr_t socket_data () const noexcept
  {
    return socket_data_;
  }


  const void *head () const noexcept
  {
    return data_;
  }


  const void *tail () const noexcept
  {
    return data_ + sizeof(data_);
  }


  void *begin () noexcept
  {
    return begin_;
  }


  void begin (size_t offset_from_head)
  {
    sal_assert(offset_from_head < sizeof(data_));
    begin_ = data_ + offset_from_head;
  }


  size_t head_gap () const noexcept
  {
    return begin_ - data_;
  }


  size_t tail_gap () const noexcept
  {
    return data_ + sizeof(data_) - end_;
  }


  const void *end () const noexcept
  {
    return end_;
  }


  size_t size () const noexcept
  {
    return end_ - begin_;
  }


  void resize (size_t s)
  {
    sal_assert(begin_ + s <= data_ + sizeof(data_));
    end_ = begin_ + s;
  }


  size_t max_size () const noexcept
  {
    return sizeof(data_);
  }


  void clear () noexcept
  {
    __bits::reset(*this);
    request_data_ = socket_data_ = 0;
    begin_ = data_;
    end_ = data_ + sizeof(data_);
  }


private:

  io_context_t * const owner_context_;
  io_context_t *this_context_{};
  uintptr_t request_data_{}, socket_data_{};
  char *begin_{};
  char *end_{};

  union
  {
    mpsc_sync_t::intrusive_queue_hook_t free_;
    no_sync_t::intrusive_queue_hook_t completed_;
  };

  static constexpr size_t members_size = sizeof(__bits::io_buf_aux_t)
    + sizeof(decltype(owner_context_))
    + sizeof(decltype(this_context_))
    + sizeof(decltype(request_data_))
    + sizeof(decltype(socket_data_))
    + sizeof(decltype(begin_))
    + sizeof(decltype(end_))
    + __bits::max_hook_size
  ;
  static constexpr size_t pad_size =
    __bits::round_next_4(members_size) != members_size
      ? __bits::round_next_4(members_size) - members_size
      : 4
  ;

  char pad_[pad_size];
  char data_[4096 - members_size - pad_size];

  using free_list = intrusive_queue_t<
    io_buf_t, mpsc_sync_t, &io_buf_t::free_
  >;
  using completed_list = intrusive_queue_t<
    io_buf_t, no_sync_t, &io_buf_t::completed_
  >;


  io_buf_t (io_context_t *owner) noexcept
    : owner_context_(owner)
  {}


  void static_check ()
  {
    static_assert(sizeof(io_buf_t) == 4096,
      "expected sizeof(io_buf_t) == 4096B"
    );
    (void)pad_;
  }


  friend class io_context_t;
};


using io_buf_ptr = std::unique_ptr<io_buf_t, void(*)(io_buf_t*)>;


} // namespace net


__sal_end
