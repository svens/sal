#pragma once

// MultiProducer / MultiConsumer queue implementation
//
// included by sal/concurrent_queue.hpp with necessary types already provided
// here we specialise concurrent_queue<> for mpmc

// MPMC is most generic producer/consumer queue and is used as fallback for
// other queues if more scalable specific implementation is not found.

// For now, MPMC uses MPSC with SC synchronised using spinlock


#include <sal/spinlock.hpp>
#include <mutex>
#include <utility>


namespace sal {
__sal_begin


template <typename T, concurrent_queue_hook<T> T::*Hook>
class concurrent_queue<T, Hook, mpmc>
{
public:

  using use_policy = mpmc;


  static constexpr bool is_lock_free () noexcept
  {
    return false;
  }


  concurrent_queue (const concurrent_queue &) = delete;
  concurrent_queue &operator= (const concurrent_queue &) = delete;


  concurrent_queue () = default;


  concurrent_queue (concurrent_queue &&that) noexcept
    : queue_(std::move(that.queue_))
  {
  }


  concurrent_queue &operator= (concurrent_queue &&that) noexcept
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
  concurrent_queue<T, Hook, mpsc> queue_{};
};


__sal_end
} // namespace sal
