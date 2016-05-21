#pragma once

/**
 * \file sal/concurrent_queue.hpp
 * Intrusive concurrent queue (FIFO)
 */

#include <sal/config.hpp>
#include <sal/spinlock.hpp>
#include <atomic>
#include <mutex>


namespace sal {
__sal_begin


/// Opaque intrusive data to hook class into concurrent_queue
using concurrent_queue_hook = volatile void *;


/**
 * Intrusive concurrent queue (FIFO).
 *
 * Queue elements of type \a T must provide \a Hook that stores opaque data
 * managed by owning queue. Any given time specific hook can be used only to
 * store element in single queue. Same hook can be used to store element in
 * different queues at different times. If application needs to store element
 * in multiple queues same time, it needs to provide multiple hooks, one per
 * holding queue.
 *
 * Being intrusive, queue itself does not deal with node allocation. It is
 * application's responsibility to handle node management and make sure that
 * while in queue, element is kept alive and it's hook is not interfered with.
 * Also, pushing and popping elements into/from queue does not copy them, just
 * hooks/unhooks using specified member \a Hook in \a T.
 *
 * Usage:
 * \code
 * class foo
 * {
 *   sal::concurrent_queue_hook hook;
 *   int a;
 *   char b;
 * };
 *
 * sal::concurrent_queue<foo, &foo::hook> queue;
 *
 * foo f;
 * queue.push(&f);
 *
 * auto fp = queue.try_pop(); // fp == &f
 * \endcode
 */
template <typename T, concurrent_queue_hook T::*Hook>
class concurrent_queue
{
public:

  concurrent_queue (const concurrent_queue &) = delete;
  concurrent_queue &operator= (const concurrent_queue &) = delete;


  /// Construct new empty queue
  concurrent_queue () noexcept
  {
    next_(sentry_) = nullptr;
  }


  /**
   * Construct new queue filled with elements moved from \a that. Using
   * \a that after move is undefined behaviour (until another queue moves it's
   * elements into \a that).
   *
   * \note Moving elements out of \a that is not thread-safe.
   */
  concurrent_queue (concurrent_queue &&that) noexcept
  {
    next_(sentry_) = nullptr;
    operator=(std::move(that));
  }


  /**
   * Move elements from \a that to \a this. Using \a that after move is
   * undefined behaviour (until another queue moves it's elements into
   * \a that).
   *
   * Elements in \a this are "forgotten" during move. If these are dynamically
   * allocated, it is applications responsibility to release them beforehand.
   *
   * \note Moving elements out of \a that is not thread-safe.
   */
  concurrent_queue &operator= (concurrent_queue &&that) noexcept
  {
    if (that.tail_ == that.sentry_)
    {
      tail_ = head_ = sentry_;
    }
    else if (that.head_ == that.sentry_)
    {
      tail_ = that.tail_.load(std::memory_order_relaxed);
      head_ = sentry_;
      next_(head_) = next_(that.head_);
    }
    else
    {
      tail_ = that.tail_.load(std::memory_order_relaxed);
      head_ = that.head_;
    }
    that.head_ = that.tail_ = nullptr;
    return *this;
  }


  /// Push new \a node into \a this
  void push (T *node) noexcept
  {
    next_(node) = nullptr;
    auto back = tail_.exchange(node);
    next_(back) = node;
  }


  /// Pop next element from queue. If empty, return nullptr
  T *try_pop () noexcept
  {
    std::lock_guard<sal::spinlock> lock(mutex_);

    auto front = head_, next = next_(front);
    if (front == sentry_)
    {
      if (!next)
      {
        return nullptr;
      }
      front = head_ = next;
      next = next_(next);
    }

    if (next)
    {
      head_ = next;
      return front;
    }

    if (front != tail_.load())
    {
      return nullptr;
    }

    push(sentry_);

    next = next_(front);
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
  T *head_ = sentry_;


  static T *&next_ (T *node) noexcept
  {
    return reinterpret_cast<T *&>(const_cast<void *&>(node->*Hook));
  }
};


__sal_end
} // namespace sal
