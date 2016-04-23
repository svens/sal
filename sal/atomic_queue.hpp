#pragma once

/**
 * \file sal/atomic_queue.hpp
 * Synchronised intrusive queues (FIFO)
 *
 * This module provides common API for multiple open-ended queue
 * implementations with different traits.
 *
 * Most generic implementation uses two-lock queue implementation presented in
 * https://www.research.ibm.com/people/m/michael/podc-1996.pdf. If there is no
 * specialisation for queue with specified traits, two-lock queue is used as
 * fallback implementation. Each implementation provides method is_lock_free()
 * indicating whether locks are involved in synchronisation.
 */

#include <sal/config.hpp>
#include <sal/spinlock.hpp>
#include <mutex>


namespace sal {
__sal_begin


/// atomic_queue usage policy
template <bool MultiProducer, bool MultiConsumer>
struct atomic_queue_use_policy
{
  /// If true, atomic_queue::push() can be used from multiple threads
  static constexpr bool multi_producer = MultiProducer;

  /// If true, atomic_queue::try_pop() can be used from multiple threads
  static constexpr bool multi_consumer = MultiConsumer;
};


/// Single producer/consumer queue type
using spsc = atomic_queue_use_policy<false, false>;

/// Multiple producers, single consumer queue type
using mpsc = atomic_queue_use_policy<true, false>;

/// Single producer, multiple consumers queue type
using spmc = atomic_queue_use_policy<false, true>;

/// Multiple producers/consumers queue type
using mpmc = atomic_queue_use_policy<true, true>;


/**
 * Intrusive hook into application provided \a T.
 * \see atomic_queue<T>
 */
template <typename T>
using atomic_queue_hook = T *;


/**
 * Synchronised intrusive queue (FIFO).
 *
 * Queue elements of type \a T must provide \a Hook that stores opaque data
 * managed by owning queue. Any given time single hook can be used only to
 * store element in single queue. Same hook can be used to store element in
 * different queues at different times. If application needs to store element
 * in multiple queues same time, it needs to provide multiple hooks, one per
 * queue.
 *
 * Being intrusive, queue itself does not deal with node allocation. It's
 * application responsibility to deal with nodes and make sure that while in
 * queue, element is kept alive and it's hook is not interfered with. Also,
 * pushing and popping elements into/from queue does not copy them, just
 * hooks/unhooks using specified member \a Hook in \a T.
 *
 * Template parameter \a UsePolicy is used to choose internally queue
 * implementation with different requirements e.g whether queue is used by
 * multiple consumers/producers. It is undefined behaviour to push/pop
 * elements from multiple threads if \a UsePolicy is for single threaded use.
 * Of course, even with single consumer/producer \a UsePolicy, application can
 * still use it from multiple threads if proper external synchronisation is
 * used.
 *
 * Usage:
 * \code
 * class foo
 * {
 *   sal::atomic_queue_hook<foo> hook;
 * };
 *
 * sal::atomic_queue<foo, &foo::hook> queue;
 *
 * foo f;
 * queue.push(&f);
 *
 * auto *fp = queue.try_pop(); // fp == &f
 * \endcode
 */
template <typename T,
  atomic_queue_hook<T> T::*Hook,
  typename UsePolicy = mpmc
>
class atomic_queue
{
public:

  /**
   * Queue traits
   * \see atomic_queue_use_policy
   */
  using use_policy = UsePolicy;


  /// Return true if no locks are involved in queue operations
  static constexpr bool is_lock_free () noexcept
  {
    return false;
  }


  atomic_queue (const atomic_queue &) = delete;
  atomic_queue &operator= (const atomic_queue &) = delete;


  /// Construct new empty queue
  atomic_queue () noexcept
  {
    head_.ptr = tail_.ptr = reinterpret_cast<T *>(&sentry_);
    next(head_.ptr) = nullptr;
  }


  /**
   * Construct new queue filled with elements moved from \a that. Using
   * \a that after move is undefined behaviour (until another queue moves it's
   * items into \a that).
   *
   * \note Moving elements out of \a that is not synchronised. It is
   * application responsibility to make sure \a that does not change during
   * move.
   */
  atomic_queue (atomic_queue &&that) noexcept
  {
    head_.ptr = reinterpret_cast<T *>(&sentry_);
    operator=(std::move(that));
  }


  /**
   * Move elements from \a that to \a this. Using \a that after move is
   * undefined behaviour (until another queue moves it's items into \a that).
   *
   * Elements in \a this are dropped before move. If they are dynamically
   * allocated, it is application's responsibility to release them beforehand.
   */
  atomic_queue &operator= (atomic_queue &&that) noexcept
  {
    next(head_.ptr) = next(that.head_.ptr);
    next(that.head_.ptr) = nullptr;
    tail_.ptr = that.tail_.ptr;
    that.tail_.ptr = nullptr;
    return *this;
  }


  /// Push new \a node to \a this queue.
  void push (T *node) noexcept
  {
    next(node) = nullptr;
    std::lock_guard<spinlock> lock(tail_.mutex);
    next(tail_.ptr) = node;
    tail_.ptr = node;
  }


  /// Pop next item from queue. If empty, nullptr is returned.
  T *try_pop () noexcept
  {
    std::lock_guard<spinlock> lock(head_.mutex);
    if (auto node = next(head_.ptr))
    {
      next(head_.ptr) = next(node);
      return node;
    }
    return nullptr;
  }


private:

  struct
  {
    spinlock mutex;
    T *ptr;
  } head_, tail_;

  char sentry_[sizeof(T)];


  static T *&next (T *n) noexcept
  {
    return n->*Hook;
  }
};


__sal_end
} // namespace sal
