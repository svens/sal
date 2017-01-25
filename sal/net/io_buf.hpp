#pragma once

/**
 * \file sal/net/io_buf.hpp
 */


#include <sal/config.hpp>
#include <sal/net/__bits/socket.hpp>
#include <sal/net/fwd.hpp>
#include <sal/assert.hpp>
#include <sal/intrusive_queue.hpp>
#include <memory>
#include <typeinfo>


__sal_begin


namespace net {


namespace __bits {


constexpr size_t round_next_256 (size_t s)
{
  return (s + 255) & ~255;
}


// for msvc poor implementation of constexpr
union hooks_t
{
  mpsc_sync_t::intrusive_queue_hook_t free;
  no_sync_t::intrusive_queue_hook_t completed;
};

constexpr size_t max_hook_size = sizeof(hooks_t);


template <typename AsyncOperation>
const size_t type_v = typeid(AsyncOperation).hash_code();



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
  : public __bits::io_buf_t
{
public:

  io_buf_t (const io_buf_t &) = delete;
  io_buf_t &operator= (const io_buf_t &) = delete;


  io_context_t &this_context () const noexcept
  {
    return *this_context_;
  }


  uintptr_t user_data () const noexcept
  {
    return user_data_;
  }


  void user_data (uintptr_t value) noexcept
  {
    user_data_ = value;
  }


  const void *head () const noexcept
  {
    return data_;
  }


  const void *tail () const noexcept
  {
    return data_ + sizeof(data_);
  }


  char *data () noexcept
  {
    return begin_;
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


  void reset () noexcept
  {
    __bits::reset(*this);
    begin_ = data_;
    end_ = data_ + sizeof(data_);
  }


  template <typename Request, typename... Args>
  Request *make_request (Args &&...args) noexcept
  {
    static_assert(sizeof(Request) <= max_request_size,
      "sizeof(Request) exceeds request data buffer"
    );
    static_assert(std::is_trivially_destructible<Request>::value,
      "expected Request to be trivially destructible"
    );
    request_type_ = __bits::type_v<Request>;
    return new(request_data_) Request(std::forward<Args>(args)...);
  }


  template <typename Result>
  Result *make_result () noexcept
  {
    if (request_type_ == __bits::type_v<Result>)
    {
      return reinterpret_cast<Result *>(request_data_);
    }
    return nullptr;
  }


private:

  io_context_t * const owner_context_;
  io_context_t *this_context_{};
  uintptr_t user_data_{};
  size_t request_type_{};
  char *begin_{};
  char *end_{};

  union
  {
    mpsc_sync_t::intrusive_queue_hook_t free_;
    no_sync_t::intrusive_queue_hook_t completed_;
  };

  static constexpr size_t members_size = sizeof(__bits::io_buf_t)
    + sizeof(decltype(owner_context_))
    + sizeof(decltype(this_context_))
    + sizeof(decltype(user_data_))
    + sizeof(decltype(request_type_))
    + sizeof(decltype(begin_))
    + sizeof(decltype(end_))
    + __bits::max_hook_size
  ;
  static constexpr size_t max_request_size =
    __bits::round_next_256(members_size) != members_size
      ? __bits::round_next_256(members_size) - members_size
      : 256
  ;

  char request_data_[max_request_size];
  char data_[4096 - members_size - max_request_size];

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
  }


  friend class io_context_t;
};


using io_buf_ptr = std::unique_ptr<io_buf_t, void(*)(io_buf_t*)>;


} // namespace net


__sal_end
