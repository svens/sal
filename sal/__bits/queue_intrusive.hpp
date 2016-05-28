#pragma once

// DO NOT INCLUDE DIRECTLY: included from sal/queue.hpp

#include <sal/config.hpp>
#include <utility>


namespace sal {
__sal_begin


struct queue_intrusive_hook
{
  void *next;

  template <typename T, queue_intrusive_hook T::*Hook>
  class queue;
};


template <typename T, queue_intrusive_hook T::*Hook>
class queue_intrusive_hook::queue
{
public:

  queue (const queue &) = delete;
  queue &operator= (const queue &) = delete;


  queue () noexcept
  {
    next_of(head_) = nullptr;
  }


  queue (queue &&that) noexcept
  {
    operator=(std::move(that));
  }


  queue &operator= (queue &&that) noexcept
  {
    next_of(head_) = next_of(that.head_);
    tail_ = that.tail_ == that.head_ ? head_ : that.tail_;
    next_of(that.head_) = that.tail_ = nullptr;
    return *this;
  }


  void push (T *node) noexcept
  {
    next_of(node) = nullptr;
    tail_ = next_of(tail_) = node;
  }


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
