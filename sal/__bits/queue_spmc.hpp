#pragma once

// Specialisation for queue<spmc>. Do not include directly

#include <sal/config.hpp>
#include <sal/spinlock.hpp>
#include <atomic>
#include <mutex>


namespace sal {
__sal_begin


template <>
struct queue_hook<concurrent_usage::spmc>
{
  volatile void *next;
};


template <typename T, queue_hook<concurrent_usage::spmc> T::*Hook>
class queue<concurrent_usage::spmc, T, Hook>
{
public:

  queue (const queue &) = delete;
  queue &operator= (const queue &) = delete;


  queue () noexcept
  {
    next_of(sentry_) = nullptr;
  }


  queue (queue &&that) noexcept
  {
    next_of(sentry_) = nullptr;
    operator=(std::move(that));
  }


  queue &operator= (queue &&that) noexcept
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
    std::lock_guard<spinlock> lock(mutex_);

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

  sal::spinlock mutex_{};
  char pad2_[cache_line - sizeof(decltype(mutex_))];
  T *head_ = sentry_;


  static T *&next_of (T *node) noexcept
  {
    return reinterpret_cast<T *&>(
      const_cast<void *&>((node->*Hook).next)
    );
  }
};


__sal_end
} // namespace sal
