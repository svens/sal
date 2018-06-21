#pragma once

// DO NOT INCLUDE DIRECTLY
// It is incuded from sal/queue.hpp

#include <atomic>
#include <deque>


__sal_begin


namespace __bits {

template <typename T>
inline T load_consume (const T *addr) noexcept
{
  T v = *const_cast<const T volatile *>(addr);
  std::atomic_thread_fence(std::memory_order_consume);
  return v;
}

template <typename T>
void store_release (T *addr, T v) noexcept
{
  std::atomic_thread_fence(std::memory_order_release);
  *const_cast<T volatile *>(addr) = v;
}

} // namespace __bits


template <typename T>
class queue_t<T, spsc_sync_t>
{
public:

  queue_t (const queue_t &) = delete;
  queue_t &operator= (const queue_t &) = delete;


  queue_t () noexcept
  {
    head_->next_ = nullptr;
  }


  queue_t (queue_t &&that) noexcept
  {
    operator=(std::move(that));
  }


  queue_t &operator= (queue_t &&that) noexcept
  {
    head_ = that.head_;
    tail_ = that.tail_;
    cache_tail_ = that.cache_tail_;
    head_copy_ = that.head_copy_;
    cache_ = std::move(that.cache_);
    that.head_ = that.tail_ = that.cache_tail_ = that.head_copy_ = nullptr;
    return *this;
  }


  void push (T v)
  {
    auto node = alloc();
    node->next_ = nullptr;
    node->value_ = v;
    __bits::store_release(&tail_->next_, node);
    tail_ = node;
  }


  bool try_pop (T *v) noexcept
  {
    if (head_->next_)
    {
      *v = head_->next_->value_;
      __bits::store_release(&head_, head_->next_);
      return true;
    }
    return false;
  }


private:

  struct node_t
  {
    node_t *next_;
    T value_;
  };
  std::deque<node_t> cache_{1};

  // consumer
  node_t *head_ = &cache_.back();
  char pad0_[__bits::hardware_destructive_interference_size];

  // producer
  node_t *tail_ = head_, *cache_tail_ = head_, *head_copy_ = head_;


  node_t *alloc ()
  {
    if (cache_tail_ != head_copy_)
    {
      auto node = cache_tail_;
      cache_tail_ = cache_tail_->next_;
      return node;
    }

    head_copy_ = __bits::load_consume(&head_);
    if (cache_tail_ != head_copy_)
    {
      auto node = cache_tail_;
      cache_tail_ = cache_tail_->next_;
      return node;
    }

    cache_.emplace_back();
    return &cache_.back();
  }
};


// not implementd
template <typename T>
class queue_t<T, mpsc_sync_t>;


__sal_end
