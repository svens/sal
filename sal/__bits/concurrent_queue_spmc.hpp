#pragma once

// SingleProducer / MultiConsumer queue implementation
//
// included by sal/concurrent_queue.hpp with necessary types already provided
// here we specialise concurrent_queue<> for spmc


#include <sal/spinlock.hpp>
#include <mutex>


namespace sal {
__sal_begin


template <typename T>
class concurrent_queue<T, spmc>
{
public:

  using use_policy = spmc;

  using impl = concurrent_queue<T, spsc>;
  using node = typename impl::node;


  concurrent_queue () = delete;
  concurrent_queue (const concurrent_queue &) = delete;
  concurrent_queue &operator= (const concurrent_queue &) = delete;


  concurrent_queue (node *stub) noexcept
    : impl_(stub)
  {}


  concurrent_queue (concurrent_queue &&that) noexcept
    : impl_(std::move(that.impl_))
  {}


  concurrent_queue &operator= (concurrent_queue &&that) noexcept
  {
    impl_ = std::move(that.impl_);
    return *this;
  }


  bool is_lock_free () noexcept
  {
    return false;
  }


  node *stub () const noexcept
  {
    return impl_.stub();
  }


  void push (node *n) noexcept
  {
    impl_.push(n);
  }


  node *try_pop () noexcept(std::is_nothrow_move_assignable<T>::value)
  {
    std::lock_guard<spinlock> lock(mutex_);
    return impl_.try_pop();
  }


private:

  spinlock mutex_{};
  impl impl_;
};


__sal_end
} // namespace sal
