#pragma once

/**
 * \file sal/intrusive_mpsc_queue.hpp
 * Intrusive lockfree multiple producers single consumer queue (FIFO).
 */

#include <sal/config.hpp>
#include <atomic>


__sal_begin


/**
 * Intrusive MPSC queue hook.
 * \see intrusive_mpsc_queue_t
 */
template <typename T>
using intrusive_mpsc_queue_hook_t = volatile T *;


/**
 * Intrusive multiple producers single consumer queue (FIFO).
 *
 * Elements of type \a T must provide member address \a Next that stores
 * opaque data managed by container. Any given time specific hook can be used
 * only to store element in single container. Same hook can be used to store
 * element in different containers at different times. If application needs to
 * store element in multiple containers same time, it needs to provide
 * multiple hooks, one per owner.
 *
 * Being intrusive, container itself does not deal with node allocation. It is
 * application's responsibility to handle node management and make sure that
 * while in container, element is kept alive and it's hook is not interfered
 * with. Also, pushing and popping elements into/from container does not copy
 * them, just hooks/unhooks using specified member \a Next in \a T.
 *
 * Usage:
 * \code
 * class foo_t
 * {
 *   sal::intrusive_mpsc_queue_hook_t<foo_t> next;
 *   int a;
 *   char b;
 * };
 * sal::intrusive_mpsc_queue_t<&foo_t::next> queue;
 *
 * foo_t f;
 * queue.push(&f);
 *
 * auto fp = queue.try_pop(); // fp == &f
 * \endcode
 *
 * \note Method push() is thread safe but not other methods.
 */
template <typename T, intrusive_mpsc_queue_hook_t<T> T::*Next>
class intrusive_mpsc_queue_t
{
public:

  intrusive_mpsc_queue_t () noexcept = default;

  intrusive_mpsc_queue_t (const intrusive_mpsc_queue_t &) = delete;
  intrusive_mpsc_queue_t &operator= (const intrusive_mpsc_queue_t &) = delete;


  /**
   * Construct new queue with elements from \a that. \a that will be empty
   * after the move.
   */
  intrusive_mpsc_queue_t (intrusive_mpsc_queue_t &&that) noexcept
  {
    operator=(std::move(that));
  }


  /**
   * Move elements of \a that into \a this. \a that will be empty after the
   * move. Existing elements of \a this are "forgotten" during move. If there
   * are dynamically allocated, it is application responsibility to release
   * them beforehand.
   */
  intrusive_mpsc_queue_t &operator= (intrusive_mpsc_queue_t &&that) noexcept
  {
    if (that.tail_ == that.sentry_)
    {
      tail_ = head_ = sentry_;
    }
    else if (that.head_ == that.sentry_)
    {
      tail_ = that.tail_.load(std::memory_order_relaxed);
      head_ = sentry_;
      head_->*Next = that.head_->*Next;
    }
    else
    {
      tail_ = that.tail_.load(std::memory_order_relaxed);
      head_ = that.head_;
    }
    that.head_ = that.tail_ = nullptr;
    return *this;
  }


  /**
   * Push new \a element to back of queue.
   */
  void push (T *node) noexcept
  {
    node->*Next = nullptr;
    auto back = tail_.exchange(node, std::memory_order_release);
    back->*Next = node;
  }


  /**
   * Pop next element from head of queue. If empty, return nullptr.
   */
  T *try_pop () noexcept
  {
    auto front = head_, next = const_cast<T *>(front->*Next);
    if (front == sentry_)
    {
      if (!next)
      {
        return nullptr;
      }
      front = head_ = next;
      next = const_cast<T *>(next->*Next);
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

    next = const_cast<T *>(front->*Next);
    if (next)
    {
      head_ = next;
      return front;
    }

    return nullptr;
  }


  /**
   * Return true if queue has no elements. This method returns reliably valid
   * result only from consumer side.
   */
  bool empty () const noexcept
  {
    return tail_.load(std::memory_order_acquire) == sentry_;
  }


private:

  T * const sentry_ = reinterpret_cast<T *>(&pad_);

  alignas(__bits::hardware_destructive_interference_size)
  __sal_warning_suppress_aligned_struct_padding
  char pad_[sizeof(T)] = { 0 };

  alignas(__bits::hardware_destructive_interference_size)
  __sal_warning_suppress_aligned_struct_padding
  std::atomic<T *> tail_{sentry_};

  alignas(__bits::hardware_destructive_interference_size)
  __sal_warning_suppress_aligned_struct_padding
  T *head_ = sentry_;
};


__sal_end
