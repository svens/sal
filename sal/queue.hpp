#pragma once

/**
 * \file sal/queue.hpp
 * Intrusive queue (FIFO) with possible concurrent usage
 */

#include <sal/config.hpp>
#include <utility>


namespace sal {
__sal_begin


/// Intrusive unsynchronised queue
struct intrusive_t;

/// Multiple producers, single consumer concurrent queue
struct mpsc_t;

/// Single producer, single consumer concurrent queue
struct spsc_t;


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
 *   sal::mpsc_t hook;
 *   int a;
 *   char b;
 * };
 *
 * sal::queue_t<foo, sal::mpsc_t, &foo::hook> queue;
 *
 * foo f;
 * queue.push(&f);
 *
 * auto fp = queue.try_pop(); // fp == &f
 * \endcode
 */
template <typename T, typename QueueHook, QueueHook T::*Hook>
class queue_t
{
public:

  queue_t (const queue_t &) = delete;
  queue_t &operator= (const queue_t &) = delete;


  queue_t () noexcept = default;


  /**
   * Construct new queue filled with elements moved from \a that. Using
   * \a that after move is undefined behaviour (until another queue moves it's
   * elements into \a that).
   *
   * \note Moving elements out of \a that is not thread-safe.
   */
  queue_t (queue_t &&that) noexcept
    : queue_(std::move(that.queue_))
  {
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
  queue_t &operator= (queue_t &&that) noexcept
  {
    queue_ = std::move(that.queue_);
    return *this;
  }


  /// Push new \a node into \a this
  void push (T *node) noexcept
  {
    queue_.push(node);
  }


  /// Pop next element from queue. If empty, return nullptr
  T *try_pop () noexcept
  {
    return queue_.try_pop();
  }


private:

  typename QueueHook::template queue_t<T, Hook> queue_{};
};


__sal_end
} // namespace sal


#include <sal/__bits/queue_intrusive.hpp>
#include <sal/__bits/queue_mpsc.hpp>
#include <sal/__bits/queue_spsc.hpp>
