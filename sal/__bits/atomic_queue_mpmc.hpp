#pragma once

// MultiProducer / MultiConsumer queue implementation
//
// included by sal/atomic_queue.hpp with necessary types already provided
// here we specialise atomic_queue<> for mpmc

// MPMC is most generic producer/consumer queue and is used as fallback for
// other queues if more scalable specific implementation is not found.


#include <sal/spinlock.hpp>
#include <mutex>


namespace sal {
__sal_begin


template <typename T, atomic_queue_hook<T> T::*Hook>
class atomic_queue<T, Hook, mpmc>
{
public:

  using use_policy = mpmc;


  static constexpr bool is_lock_free () noexcept
  {
    return false;
  }


  atomic_queue (const atomic_queue &) = delete;
  atomic_queue &operator= (const atomic_queue &) = delete;


  atomic_queue () noexcept
  {
    next_(head_) = nullptr;
  }


  atomic_queue (atomic_queue &&that) noexcept
  {
    operator=(std::move(that));
  }


  atomic_queue &operator= (atomic_queue &&that) noexcept
  {
    next_(head_) = next_(that.head_);
    tail_ = next_(head_) ? that.tail_ : head_;
    return *this;
  }


  void push (T *node) noexcept
  {
    next_(node) = nullptr;
    std::lock_guard<spinlock> lock(mutex_);
    tail_ = next_(tail_) = node;
  }


  T *try_pop () noexcept
  {
    std::lock_guard<spinlock> lock(mutex_);
    if (auto node = next_(head_))
    {
      next_(head_) = next_(node);
      if (!next_(head_))
      {
        tail_ = head_;
      }
      return node;
    }
    return nullptr;
  }


private:

  spinlock mutex_{};

  char sentry_[sizeof(T)];
  T * const head_ = reinterpret_cast<T *>(&sentry_);
  T *tail_ = head_;

  static T *&next_ (T *n) noexcept
  {
    return n->*Hook;
  }
};


__sal_end
} // namespace sal
