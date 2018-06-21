#pragma once

// DO NOT INCLUDE DIRECTLY
// It is incuded from sal/intrusive_queue.hpp

#include <atomic>


__sal_begin


template <typename T, spsc_sync_t::intrusive_queue_hook_t T::*Hook>
class intrusive_queue_t<T, spsc_sync_t, Hook>
{
public:

  intrusive_queue_t (const intrusive_queue_t &) = delete;
  intrusive_queue_t &operator= (const intrusive_queue_t &) = delete;


  intrusive_queue_t () noexcept = default;


  intrusive_queue_t (intrusive_queue_t &&that) noexcept
  {
    operator=(std::move(that));
  }


  intrusive_queue_t &operator= (intrusive_queue_t &&that) noexcept
  {
    tail_ = that.tail_;
    that.tail_ = nullptr;

    head_ = that.head_;
    that.head_ = nullptr;

    seq_ = that.seq_;
    last_seq_ = that.last_seq_;

    return *this;
  }


  void push (T *node) noexcept
  {
    const auto seq = seq_ + 1;
    seq_of(node) = seq;
    next_of(node) = tail_;
    tail_ = node;
    seq_ = seq;
  }


  T *try_pop () noexcept
  {
    if (auto node = head_)
    {
      head_ = const_cast<T *>(next_of(head_));
      return node;
    }

    const auto seq = seq_;
    if (seq != last_seq_)
    {
      return reverse_and_pop(seq, const_cast<T *>(tail_));
    }

    return nullptr;
  }


  bool empty () const noexcept
  {
    return !tail_ || (!head_ && seq_ == last_seq_);
  }


private:

  volatile T *tail_{nullptr};
  volatile unsigned seq_{0};

  char pad0_[__bits::hardware_destructive_interference_size
    - sizeof(decltype(tail_))
    - sizeof(decltype(seq_))
  ];

  T *head_{nullptr};
  unsigned last_seq_{0};


  static volatile unsigned &seq_of (T *node) noexcept
  {
    return (node->*Hook).seq;
  }


  static volatile T *&next_of (T *node) noexcept
  {
    return reinterpret_cast<volatile T *&>((node->*Hook).next);
  }


  T *reverse_and_pop (unsigned seq, T *head) noexcept
  {
    while (seq_of(head) != seq)
    {
      head = const_cast<T *>(next_of(head));
    }

    const auto last_seq = last_seq_ + 1;
    while (seq_of(head) != last_seq)
    {
      auto next = next_of(head);
      next_of(head) = head_;
      head_ = head;
      head = const_cast<T *>(next);
    }
    last_seq_ = seq;

    return head;
  }
};


template <typename T, mpsc_sync_t::intrusive_queue_hook_t T::*Hook>
class intrusive_queue_t<T, mpsc_sync_t, Hook>
{
public:

  intrusive_queue_t (const intrusive_queue_t &) = delete;
  intrusive_queue_t &operator= (const intrusive_queue_t &) = delete;


  intrusive_queue_t () noexcept
  {
    next_of(sentry_) = nullptr;
  }


  intrusive_queue_t (intrusive_queue_t &&that) noexcept
    : intrusive_queue_t()
  {
    operator=(std::move(that));
  }


  intrusive_queue_t &operator= (intrusive_queue_t &&that) noexcept
  {
    if (that.tail_ == that.sentry_)
    {
      tail_ = head_ = sentry_;
    }
    else if (that.head_ == that.sentry_)
    {
      tail_ = that.tail_.load(std::memory_order_relaxed);
      head_ = sentry_;
      next_of(head_) = next_of(that.head_);
    }
    else
    {
      tail_ = that.tail_.load(std::memory_order_relaxed);
      head_ = that.head_;
    }
    that.head_ = that.tail_ = nullptr;
    return *this;
  }


  void push (T *node) noexcept
  {
    next_of(node) = nullptr;
    auto back = tail_.exchange(node, std::memory_order_release);
    next_of(back) = node;
  }


  T *try_pop () noexcept
  {
    auto front = head_, next = next_of(front);
    if (front == sentry_)
    {
      if (!next)
      {
        return nullptr;
      }
      front = head_ = next;
      next = next_of(next);
    }

    if (next)
    {
      head_ = next;
      return front;
    }

    if (front != tail_.load(std::memory_order_acquire))
    {
      return nullptr;
    }

    push(sentry_);

    next = next_of(front);
    if (next)
    {
      head_ = next;
      return front;
    }

    return nullptr;
  }


  bool empty () const noexcept
  {
    return tail_.load(std::memory_order_acquire) == sentry_;
  }


private:

  // using pad0_ also as sentry_
  T * const sentry_ = reinterpret_cast<T *>(&pad0_);
  char pad0_[
    sizeof(T) < __bits::hardware_destructive_interference_size - sizeof(decltype(sentry_))
    ? __bits::hardware_destructive_interference_size - sizeof(decltype(sentry_))
    : sizeof(T)
  ];

  std::atomic<T *> tail_{sentry_};
  char pad1_[
    __bits::hardware_destructive_interference_size - sizeof(decltype(tail_))
  ];

  T *head_ = sentry_;


  static T *&next_of (T *node) noexcept
  {
    return reinterpret_cast<T *&>(const_cast<void *&>(node->*Hook));
  }
};


__sal_end
