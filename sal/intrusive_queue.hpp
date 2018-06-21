#pragma once

/**
 * \file sal/intrusive_queue.hpp
 * Intrusive queue (FIFO).
 */

#include <sal/config.hpp>


__sal_begin


/**
 * Intrusive queue hook.
 * \see intrusive_queue_t
 */
template <typename T>
using intrusive_queue_hook_t = T *;


/**
 * Intrusive queue (FIFO).
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
 *   sal::intrusive_queue_hook_t<foo_t> next;
 *   int a;
 *   char b;
 * };
 * sal::intrusive_queue_t<&foo_t::next> queue;
 *
 * foo_t f;
 * queue.push(&f);
 *
 * auto fp = queue.try_pop(); // fp == &f
 * \endcode
 *
 * \note This container is not thread safe.
 */
template <typename T, intrusive_queue_hook_t<T> T::*Next>
class intrusive_queue_t
{
public:

  intrusive_queue_t () noexcept = default;

  intrusive_queue_t (const intrusive_queue_t &) = delete;
  intrusive_queue_t &operator= (const intrusive_queue_t &) = delete;


  /**
   * Construct new queue with elements from \a that. \a that will be empty
   * after the move.
   */
  intrusive_queue_t (intrusive_queue_t &&that) noexcept
  {
    operator=(std::move(that));
  }


  /**
   * Move elements of \a that into \a this. \a that will be empty after the
   * move. Existing elements of \a this are "forgotten" during move. If there
   * are dynamically allocated, it is application responsibility to release
   * them beforehand.
   */
  intrusive_queue_t &operator= (intrusive_queue_t &&that) noexcept
  {
    head_->*Next = that.head_->*Next;
    tail_ = that.tail_ == that.head_ ? head_ : that.tail_;
    that.head_->*Next = that.tail_ = nullptr;
    return *this;
  }


  /**
   * Push new \a element to back of queue.
   */
  void push (T *node) noexcept
  {
    node->*Next = nullptr;
    tail_ = tail_->*Next = node;
  }


  /**
   * Pop next element from head of queue. If empty, return nullptr.
   */
  T *try_pop () noexcept
  {
    if (auto node = head_->*Next)
    {
      head_->*Next = node->*Next;
      if (head_->*Next == nullptr)
      {
        tail_ = head_;
      }
      return node;
    }
    return nullptr;
  }


  /**
   * Return true if queue has no elements.
   */
  bool empty () const noexcept
  {
    return tail_ == reinterpret_cast<const T *>(&sentry_);
  }


private:

  char sentry_[sizeof(T)];
  T * const head_{reinterpret_cast<T *>(&sentry_)};
  T *tail_{head_};
};


__sal_end
