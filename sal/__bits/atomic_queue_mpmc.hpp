#pragma once

// MultiProducer / MultiConsumer queue implementation
//
// included by sal/atomic_queue.hpp with necessary types already provided
// here we specialise atomic_queue<> for mpmc

// MPMC is most generic producer/consumer queue and is used as fallback for
// other queues if more scalable specific implementation is not found.

// For now, MPMC uses MPSC with SC synchronised using spinlock


#include <sal/spinlock.hpp>
#include <mutex>
#include <utility>


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


  atomic_queue () = default;


  atomic_queue (atomic_queue &&that) noexcept
    : queue_(std::move(that.queue_))
  {
  }


  atomic_queue &operator= (atomic_queue &&that) noexcept
  {
    queue_ = std::move(that.queue_);
    return *this;
  }


  void push (T *node) noexcept
  {
    queue_.push(node);
  }


  T *try_pop () noexcept
  {
    std::lock_guard<spinlock> lock(mutex_);
    return queue_.try_pop();
  }


private:

  spinlock mutex_{};
  atomic_queue<T, Hook, mpsc> queue_{};
};


__sal_end
} // namespace sal
