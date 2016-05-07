#pragma once

// MultiProducer / SingleConsumer queue implementation
//
// included by sal/concurrent_queue.hpp with necessary types already provided
// here we specialise concurrent_queue<> for mpsc

//
// Contains modified code from:
//

// http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue
//
// Copyright (c) 2010-2011 Dmitry Vyukov. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY DMITRY VYUKOV "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
// NO EVENT SHALL DMITRY VYUKOV OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// The views and conclusions contained in the software and documentation are
// those of the authors and should not be interpreted as representing official
// policies, either expressed or implied, of Dmitry Vyukov.


#include <sal/spinlock.hpp>
#include <mutex>


namespace sal {
__sal_begin


template <typename T>
class concurrent_queue<T, mpsc>
{
public:

  using use_policy = mpsc;


  struct node
  {
    volatile node *next{};
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
    head_ = that.head_;
    tail_ = that.tail_.load(std::memory_order_relaxed);
    that.head_ = that.tail_ = nullptr;
    return *this;
  }


  bool is_lock_free () noexcept
  {
    return true;
  }


  node *stub () const noexcept
  {
    return head_;
  }


  void push (node *n) noexcept
  {
    n->next = nullptr;
    auto back = tail_.exchange(n, std::memory_order_acq_rel);
    back->next = n;
  }


  node *try_pop () noexcept(std::is_nothrow_move_assignable<T>::value)
  {
    auto front = head_;
    if (auto next = front->next)
    {
      head_ = const_cast<node *>(next);
      front->data = std::move(next->data);
      return front;
    }
    return nullptr;
  }


private:

  alignas(64) node *head_;
  alignas(64) volatile std::atomic<node *> tail_;
};


__sal_end
} // namespace sal
