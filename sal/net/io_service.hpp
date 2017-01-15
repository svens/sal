#pragma once

/**
 * \file sal/net/io_service.hpp
 */


#include <sal/config.hpp>
#include <sal/net/__bits/socket.hpp>
#include <sal/intrusive_queue.hpp>
#include <algorithm>
#include <deque>
#include <memory>


#include <iostream>
#include <chrono>


__sal_begin


namespace net {


class io_buf_t;
class io_context_t;


namespace __bits {


constexpr size_t round_next_4 (size_t s)
{
  return (s + 3) & ~3;
}


// for msvc poor implementation of constexpr
union hooks_t
{
  mpsc_sync_t::intrusive_queue_hook_t free;
  no_sync_t::intrusive_queue_hook_t deferred;
};
constexpr size_t max_hook_size = sizeof(hooks_t);


#if __sal_os_windows

using io_buf_aux_t = OVERLAPPED;

void reset (io_buf_aux_t &aux) noexcept
{
  std::memset(&aux, '\0', sizeof(aux));
}

#else

struct io_buf_aux_t
{};

void reset (io_buf_aux_t &) noexcept
{}

#endif


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

  void *head () noexcept
  {
    return data_;
  }


  void *begin () noexcept
  {
    return begin_;
  }


  const void *end () const noexcept
  {
    return end_;
  }


  const void *tail () const noexcept
  {
    return data_ + sizeof(data_);
  }


  size_t size () const noexcept
  {
    return end_ - begin_;
  }


  void resize (size_t s) noexcept
  {
    (void)s;
  }


  size_t max_size () const noexcept
  {
    return sizeof(data_);
  }


  size_t capacity () const noexcept
  {
    return max_size();
  }


  void clear () noexcept
  {
    __bits::reset(*this);
    begin_ = data_;
    end_ = data_ + sizeof(data_);
  }


private:

  io_context_t *owner_;

  char *begin_{};
  char *end_{};

  union
  {
    mpsc_sync_t::intrusive_queue_hook_t free_;
    no_sync_t::intrusive_queue_hook_t deferred_;
  };

  static constexpr size_t members_size = sizeof(__bits::io_buf_aux_t)
    + sizeof(decltype(owner_))
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
  using deferred_list = intrusive_queue_t<
    io_buf_t, no_sync_t, &io_buf_t::deferred_
  >;


  io_buf_t (io_context_t *owner) noexcept
    : owner_(owner)
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


class io_context_t
{
public:

  ~io_context_t ()
  {
    std::cout << "*** " << pool_.size() << '\n';
  }


  io_buf_ptr make_buf ()
  {
    auto *io_buf = free_bufs_.try_pop();
    if (!io_buf)
    {
      io_buf = extend_and_alloc();
    }
    io_buf->clear();
    return io_buf_ptr{io_buf, &io_context_t::free_io_buf};
  }


  void test ()
  {
    using clock_type = std::chrono::high_resolution_clock;
    using milliseconds = std::chrono::milliseconds;

    auto begin = clock_type::now();
    for (size_t i = 0;  i != 10'000'000;  ++i)
    {
      auto x = make_buf();
      *static_cast<char *>(x->begin()) = 'a';
    }
    auto end = clock_type::now();
    auto msec = std::chrono::duration_cast<milliseconds>(end - begin);
    std::cout << "ms=" << msec.count() << '\n';
  }


private:

  using io_buf_block_t = char[1024 * sizeof(io_buf_t)];
  std::deque<io_buf_block_t> pool_;
  io_buf_t::free_list free_bufs_{};


  io_buf_t *extend_and_alloc ()
  {
    pool_.emplace_back();
    char *it = pool_.back(), * const e = it + sizeof(io_buf_block_t);
    for (/**/;  it != e;  it += sizeof(io_buf_t))
    {
      auto *io_buf = new(it) io_buf_t(this);
      free_bufs_.push(io_buf);
    }
    return free_bufs_.try_pop();
  }


  static void free_io_buf (io_buf_t *io_buf) noexcept
  {
    io_buf->owner_->free_bufs_.push(io_buf);
  }
};


} // namespace net


__sal_end
