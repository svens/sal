#pragma once

/**
 * \file sal/intrusive_queue.hpp
 * Intrusive queue (FIFO) with possible concurrent usage
 */

#include <sal/config.hpp>
#include <sal/sync_policy.hpp>
#include <utility>


namespace sal {
__sal_begin


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
 *   sal::mpsc_sync_t::intrusive_queue_hook_t hook;
 *   int a;
 *   char b;
 * };
 *
 * sal::queue_t<foo, sal::mpsc_sync_t, &foo::hook> queue;
 *
 * foo f;
 * queue.push(&f);
 *
 * auto fp = queue.try_pop(); // fp == &f
 * \endcode
 */
template <typename T,
  typename SyncPolicy,
  typename SyncPolicy::intrusive_queue_hook_t T::*Hook
>
class intrusive_queue_t
{
public:

  intrusive_queue_t (const intrusive_queue_t &) = delete;
  intrusive_queue_t &operator= (const intrusive_queue_t &) = delete;


  // Construct new queue with no elements
  intrusive_queue_t () noexcept
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
  intrusive_queue_t (intrusive_queue_t &&that) noexcept
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
  intrusive_queue_t &operator= (intrusive_queue_t &&that) noexcept
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
      if (!next_of(head_))
      {
        tail_ = head_;
      }
      return node;
    }
    return nullptr;
  }


private:

  char sentry_[sizeof(T)];
  T * const head_{reinterpret_cast<T *>(&sentry_)};
  T *tail_{head_};

  static T *&next_of (T *node) noexcept
  {
    return reinterpret_cast<T *&>(node->*Hook);
  }
};


__sal_end
} // namespace sal


// specializations for synchronised intrusive queues
#include <sal/__bits/intrusive_queue.hpp>
