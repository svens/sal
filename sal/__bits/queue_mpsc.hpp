#pragma once

// DO NOT INCLUDE DIRECTLY: included from sal/queue.hpp

#include <sal/config.hpp>
#include <atomic>
#include <utility>


namespace sal {
__sal_begin


struct mpsc_t
{
  volatile void *hook_next;

  template <typename T, mpsc_t T::*Hook>
  class queue_t;
};


template <typename T, mpsc_t T::*Hook>
class mpsc_t::queue_t
{
public:

  queue_t (const queue_t &) = delete;
  queue_t &operator= (const queue_t &) = delete;


  queue_t () noexcept
  {
    next_of(sentry_) = nullptr;
  }


  queue_t (queue_t &&that) noexcept
  {
    next_of(sentry_) = nullptr;
    operator=(std::move(that));
  }


  queue_t &operator= (queue_t &&that) noexcept
  {
    if (that.tail_ == that.sentry_)
    {
      tail_ = head_ = sentry_;
    }
    else if (that.head_ == that.sentry_)
    {
      tail_ = that.tail_.load(std::memory_order_relaxed);
      head_ = sentry_;
      next_of(head_) = next_of(that.head_);
    }
    else
    {
      tail_ = that.tail_.load(std::memory_order_relaxed);
      head_ = that.head_;
    }
    that.head_ = that.tail_ = nullptr;
    return *this;
  }


  void push (T *node) noexcept
  {
    next_of(node) = nullptr;
    auto back = tail_.exchange(node, std::memory_order_release);
    next_of(back) = node;
  }


  T *try_pop () noexcept
  {
    auto front = head_, next = next_of(front);
    if (front == sentry_)
    {
      if (!next)
      {
        return nullptr;
      }
      front = head_ = next;
      next = next_of(next);
    }

    if (next)
    {
      head_ = next;
      return front;
    }

    if (front != tail_.load(std::memory_order_acquire))
    {
      return nullptr;
    }

    push(sentry_);

    next = next_of(front);
    if (next)
    {
      head_ = next;
      return front;
    }

    return nullptr;
  }


private:

  // TODO: std::hardware_destructive_interference_size
  static constexpr size_t cache_line = 64;

  // using pad0_ also as sentry_
  T * const sentry_ = reinterpret_cast<T *>(&pad0_);
  char pad0_[sizeof(T) < cache_line - sizeof(decltype(sentry_))
    ? cache_line - sizeof(decltype(sentry_))
    : sizeof(T)
  ];

  alignas(cache_line) std::atomic<T *> tail_{sentry_};
  char pad1_[cache_line - sizeof(decltype(tail_))];

  T *head_ = sentry_;


  static T *&next_of (T *node) noexcept
  {
    return reinterpret_cast<T *&>(
      const_cast<void *&>((node->*Hook).hook_next)
    );
  }
};


__sal_end
} // namespace sal
