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


template <typename T>
class concurrent_queue<T, mpmc>
{
public:

  using use_policy = mpmc;


  struct node
  {
    node *next = nullptr;
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
  {
    stub->next = nullptr;
    head_.ptr = tail_.ptr = stub;
  }


  concurrent_queue (concurrent_queue &&that) noexcept
  {
    operator=(std::move(that));
  }


  concurrent_queue &operator= (concurrent_queue &&that) noexcept
  {
    head_.ptr = that.head_.ptr;
    tail_.ptr = that.tail_.ptr;
    that.head_.ptr = that.tail_.ptr = nullptr;
    return *this;
  }


  bool is_lock_free () const noexcept
  {
    return false;
  }


  node *stub () const noexcept
  {
    return head_.ptr;
  }


  void push (node *n) noexcept
  {
    n->next = nullptr;
    std::lock_guard<spinlock> lock(tail_.mutex);
    tail_.ptr = tail_.ptr->next = n;
  }


  node *try_pop () noexcept(std::is_nothrow_move_assignable<T>::value)
  {
    std::lock_guard<spinlock> lock(head_.mutex);
    if (auto next = head_.ptr->next)
    {
      auto front = head_.ptr;
      front->data = std::move(next->data);
      head_.ptr = next;
      return front;
    }
    return nullptr;
  }


private:

  struct
  {
    spinlock mutex{};
    node *ptr;
    char pad[64 - sizeof(spinlock) - sizeof(node *)];
  } head_{}, tail_{};
};


__sal_end
} // namespace sal
