#pragma once

// MultiProducer / SingleConsumer queue implementation
//
// included by sal/atomic_queue.hpp with necessary types already provided
// here we specialise atomic_queue<> for mpsc

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


#include <atomic>
#include <utility>


namespace sal {
__sal_begin


template <typename T, atomic_queue_hook<T> T::*Hook>
class atomic_queue<T, Hook, mpsc>
{
public:

  using use_policy = mpsc;


  static constexpr bool is_lock_free () noexcept
  {
    return true;
  }


  atomic_queue (const atomic_queue &) = delete;
  atomic_queue &operator= (const atomic_queue &) = delete;


  atomic_queue () noexcept
  {
    tail_->*Hook = nullptr;
  }


  atomic_queue (atomic_queue &&that) noexcept
  {
    tail_->*Hook = nullptr;
    operator=(std::move(that));
  }


  atomic_queue &operator= (atomic_queue &&that) noexcept
  {
    if (that.head_ == that.sentry_)
    {
      head_ = tail_ = sentry_;
    }
    else if (that.tail_ == that.sentry_)
    {
      head_ = that.head_;
      tail_ = sentry_;
      tail_->*Hook = that.tail_->*Hook;
    }
    else
    {
      head_ = that.head_;
      tail_ = that.tail_;
    }
    that.head_ = that.tail_ = nullptr;
    return *this;
  }


  void push (T *node) noexcept
  {
    node->*Hook = nullptr;
    T *prev = head_.exchange(node, std::memory_order_acq_rel);
    prev->*Hook = node;
  }


  T *try_pop () noexcept
  {
    auto tail = tail_;
    auto next = tail->*Hook;

    if (tail == sentry_)
    {
      if (!next)
      {
        return nullptr;
      }
      tail = tail_ = const_cast<T *>(next);
      next = next->*Hook;
    }

    if (next)
    {
      tail_ = const_cast<T *>(next);
      return tail;
    }

    auto head = head_;
    if (tail != head)
    {
      // TODO: can't hit this line with single-threaded unittest
      return nullptr;
    }

    push(sentry_);

    next = tail->*Hook;
    if (next)
    {
      tail_ = const_cast<T *>(next);
      return tail;
    }

    return nullptr;
  }


private:

  char stub_[sizeof(T)];
  T * const sentry_ = reinterpret_cast<T *>(&stub_);

  volatile std::atomic<T *> head_{sentry_};
  T *tail_ = sentry_;
};


__sal_end
} // namespace sal
