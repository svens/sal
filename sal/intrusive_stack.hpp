#pragma once

/**
 * \file sal/intrusive_stack.hpp
 * Intrusive stack (LIFO).
 */

#include <sal/config.hpp>
#include <utility>


__sal_begin


/**
 * Intrusive stack hook.
 * \see intrusive_stack_t
 */
template <typename T>
using intrusive_stack_hook_t = T *;


/**
 * Intrusive stack (LIFO).
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
 *   sal::intrusive_stack_hook_t<foo> next;
 *   int a;
 *   char b;
 * };
 * sal::intrusive_stack_t<&foo::next> stack;
 *
 * foo f;
 * stack.push(&f);
 *
 * auto fp = stack.try_pop(); // fp == &f
 * \endcode
 *
 * \note This container is not thread safe.
 */
template <typename T, intrusive_stack_hook_t<T> T::*Next>
class intrusive_stack_t
{
public:

  intrusive_stack_t () noexcept = default;

  intrusive_stack_t (const intrusive_stack_t &) = delete;
  intrusive_stack_t &operator= (const intrusive_stack_t &) = delete;


  /**
   * Construct new stack with elements from \a that. \a that will be empty
   * after the move.
   */
  intrusive_stack_t (intrusive_stack_t &&that) noexcept
    : top_(that.top_)
  {
    that.top_ = nullptr;
  }


  /**
   * Move elements of \a that into \a this. \a that will be empty after the
   * move. Existing elements of \a this are "forgotten" during move. If there
   * are dynamically allocated, it is application responsibility to release
   * them beforehand.
   */
  intrusive_stack_t &operator= (intrusive_stack_t &&that) noexcept
  {
    top_ = nullptr;
    swap(*this, that);
    return *this;
  }


  /**
   * Swap elements of \a a and \a b.
   */
  friend inline void swap (intrusive_stack_t &a, intrusive_stack_t &b) noexcept
  {
    using std::swap;
    swap(a.top_, b.top_);
  }


  /**
   * Push new \a element to top of stack.
   */
  void push (T *element) noexcept
  {
    element->*Next = top_;
    top_ = element;
  }


  /**
   * Pop next element from top of stack. If there is no elements in stack,
   * nullptr is returned.
   */
  T *try_pop () noexcept
  {
    auto element = top_;
    if (element)
    {
      top_ = top_->*Next;
    }
    return element;
  }


  /**
   * Return true if stack is empty.
   */
  bool empty () const noexcept
  {
    return top_ == nullptr;
  }


private:

  T *top_{nullptr};
};


__sal_end
