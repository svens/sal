#pragma once

/**
 * \file sal/queue.hpp
 * Queue (FIFO) with possible concurrent usage
 */

#include <sal/config.hpp>
#include <sal/sync_policy.hpp>
#include <queue>


namespace sal {
__sal_begin


/**
 * Queue (FIFO) with possible concurrent usage.
 *
 * This class provides methods to push elements on the back of queue and pops
 * them from front. Default implementation provides no synchronisation
 * guarantees i.e. it is undefined behaviour to access it from multiple
 * threads without explicit external locking.
 *
 * Specify one of synchronisation policies from sal/sync_policy.hpp to get
 * synchronised queue implementation. Currently only sal::spsc_sync_t is
 * implemented.
 */
template <typename T, typename SyncPolicy>
class queue_t
{
public:

  queue_t (const queue_t &) = delete;
  queue_t &operator= (const queue_t &) = delete;


  queue_t () = default;


  /**
   * Construct new queue and move elements from \a that to \a this
   */
  queue_t (queue_t &&that)
    : impl_(std::move(that.impl_))
  {}


  /**
   * Drop existing elements and acquire elements from \a that.
   */
  queue_t &operator= (queue_t &&that)
  {
    impl_ = std::move(that.impl_);
    return *this;
  }


  /**
   * Add new element \a node to back of queue
   */
  void push (T node)
  {
    impl_.emplace(node);
  }


  /**
   * Try to get element from head of queue into \a v. On success, return true.
   * If there are no elements in queue, \a v is not modified and false is
   * returned.
   */
  bool try_pop (T *v)
  {
    if (impl_.size())
    {
      *v = impl_.front();
      impl_.pop();
      return true;
    }
    return false;
  }


private:

  std::queue<T> impl_{};
};


__sal_end
} // namespace sal


// specializations for synchronised queues
#include <sal/__bits/queue.hpp>
