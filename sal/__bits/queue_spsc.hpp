#pragma once

// DO NOT INCLUDE DIRECTLY: included from sal/queue.hpp

#include <sal/config.hpp>
#include <utility>


namespace sal {
__sal_begin


struct queue_spsc_hook
{
  volatile void *hook_next;
  volatile uint32_t hook_seq;

  template <typename T, queue_spsc_hook T::*Hook>
  class queue;
};


template <typename T, queue_spsc_hook T::*Hook>
class queue_spsc_hook::queue
{
public:

  queue (const queue &) = delete;
  queue &operator= (const queue &) = delete;


  queue () noexcept = default;


  queue (queue &&that) noexcept
  {
    operator=(std::move(that));
  }


  queue &operator= (queue &&that) noexcept
  {
    tail_ = that.tail_;
    head_ = that.head_;
    that.tail_ = that.head_ = nullptr;

    seq_ = that.seq_;
    last_seq_ = that.last_seq_;

    return *this;
  }


  void push (T *node) noexcept
  {
    const auto seq = seq_ + 1;
    seq_of(node) = seq;
    next_of(node) = (tail_);
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


private:

  // TODO: std::hardware_destructive_interference_size
  static constexpr size_t cache_line = 64;

  volatile T *tail_ = nullptr;
  volatile uint32_t seq_ = 0;

  char pad0_[cache_line - sizeof(decltype(tail_)) - sizeof(decltype(seq_))];

  T *head_ = nullptr;
  uint32_t last_seq_ = 0;


  static volatile uint32_t &seq_of (T *node) noexcept
  {
    return (node->*Hook).hook_seq;
  }


  static volatile T *&next_of (T *node) noexcept
  {
    return reinterpret_cast<volatile T *&>((node->*Hook).hook_next);
  }


  T *reverse_and_pop (uint32_t seq, T *head) noexcept
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


__sal_end
} // namespace sal
