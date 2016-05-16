#pragma once

// SingleProducer / SingleConsumer queue implementation
//
// included by sal/concurrent_queue.hpp with necessary types already provided
// here we specialise concurrent_queue<> for spsc


#include <atomic>
#include <mutex>
#include <utility>


namespace sal {
__sal_begin


template <typename T>
class concurrent_queue<T, spsc>
{
public:

  using use_policy = spsc;


  struct node
  {
    std::atomic<node *> next{};
    T data{};

    node (const T &data)
      : data(data)
    {}

    template <typename... Args>
    node (Args &&...args)
      : data(std::forward<Args>(args)...)
    {}
  };


  concurrent_queue () = delete;
  concurrent_queue (const concurrent_queue &) = delete;
  concurrent_queue &operator= (const concurrent_queue &) = delete;


  concurrent_queue (node *stub) noexcept
    : head_(stub)
    , tail_(stub)
  {
    stub->next = nullptr;
  }


  concurrent_queue (concurrent_queue &&that) noexcept
  {
    operator=(std::move(that));
  }


  concurrent_queue &operator= (concurrent_queue &&that) noexcept
  {
    head_ = that.head_.load(std::memory_order_relaxed);
    tail_ = that.tail_.load(std::memory_order_relaxed);
    that.head_ = that.tail_ = nullptr;
    return *this;
  }


  bool is_lock_free () noexcept
  {
    // LCOV_EXCL_BR_START
    return head_.is_lock_free() && tail_.is_lock_free();
    // LCOV_EXCL_BR_STOP
  }


  node *stub () const noexcept
  {
    return head_.load(std::memory_order_acquire);
  }


  void push (node *n) noexcept
  {
    n->next.store(nullptr, std::memory_order_relaxed);
    auto back = tail_.load(std::memory_order_relaxed);
    back->next.store(n, std::memory_order_release);
    tail_.store(n, std::memory_order_relaxed);
  }


  node *try_pop () noexcept(std::is_nothrow_move_assignable<T>::value)
  {
    auto front = head_.load(std::memory_order_relaxed);
    if (auto next = front->next.load(std::memory_order_acquire))
    {
      front->data = std::move(next->data);
      head_.store(next, std::memory_order_relaxed);
      return front;
    }
    return nullptr;
  }


private:

  std::atomic<node *> head_;
  char pad0_[64 - sizeof(decltype(head_))];
  std::atomic<node *> tail_;
};


__sal_end
} // namespace sal
