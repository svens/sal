#pragma once

/**
 * \file sal/queue.hpp
 * Intrusive queue (FIFO) with possible concurrent usage
 */

#include <sal/config.hpp>
#include <utility>


namespace sal {
__sal_begin


/**
 * Concurrent usage policy. Values are ordered from strongest promises to
 * weaker but better performing.
 */
enum class concurrent_usage
{
  mpmc, ///< Multiple producers, multiple consumers
  mpsc, ///< Multiple producers, single consumer
  spmc, ///< Single producer, multiple consumers
  spsc, ///< Single producer, single consumer
  none, ///< Single-threaded, must be externally synchronised
};


/// Opaque intrusive data to hook class into queue
template <concurrent_usage ConcurrentUsage>
struct queue_hook
{
  /// Internal opaque data
  void *next;
};


/**
 * Intrusive queue (FIFO) with possible concurrent usage.
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
 *   sal::queue_hook<sal::concurrent_usage::none> hook;
 *   int a;
 *   char b;
 * };
 *
 * sal::queue<sal::concurrent_usage::none, foo, &foo::hook> queue;
 *
 * foo f;
 * queue.push(&f);
 *
 * auto fp = queue.try_pop(); // fp == &f
 * \endcode
 */
template <concurrent_usage ConcurrentUsage,
  typename T,
  queue_hook<ConcurrentUsage> T::*Hook
>
class queue
{
public:

  queue (const queue &) = delete;
  queue &operator= (const queue &) = delete;


  /// Construct new empty queue
  queue () noexcept
  {
    next_of(head_) = nullptr;
  }


  /**
   * Construct new queue filled with elements moved from \a that. Using
   * \a that after move is undefined behaviour (until another queue moves it's
   * elements into \a that).
   *
   * \note Moving elements out of \a that is not thread-safe.
   */
  queue (queue &&that) noexcept
  {
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
  queue &operator= (queue &&that) noexcept
  {
    next_of(head_) = next_of(that.head_);
    tail_ = that.tail_ == that.head_ ? head_ : that.tail_;
    next_of(that.head_) = that.tail_ = nullptr;
    return *this;
  }


  /// Push new \a node into \a this
  void push (T *node) noexcept
  {
    next_of(node) = nullptr;
    tail_ = next_of(tail_) = node;
  }


  /// Pop next element from queue. If empty, return nullptr
  T *try_pop () noexcept
  {
    if (auto node = next_of(head_))
    {
      next_of(head_) = next_of(node);
      if (next_of(head_) == nullptr)
      {
        tail_ = head_;
      }
      return node;
    }
    return nullptr;
  }


private:

  char sentry_[sizeof(T)];
  T * const head_ = reinterpret_cast<T *>(&sentry_);
  T *tail_ = head_;

  static T *&next_of (T *node) noexcept
  {
    return reinterpret_cast<T *&>((node->*Hook).next);
  }
};


__sal_end
} // namespace sal


#include <sal/__bits/queue_mpmc.hpp>
#include <sal/__bits/queue_mpsc.hpp>
#include <sal/__bits/queue_spmc.hpp>
#include <sal/__bits/queue_spsc.hpp>
