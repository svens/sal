#pragma once

// SingleProducer / SingleConsumer queue implementation
//
// included by sal/concurrent_queue.hpp with necessary types already provided
// here we specialise concurrent_queue<> for spsc


namespace sal {
__sal_begin


template <typename T, concurrent_queue_hook<T> T::*Hook>
class concurrent_queue<T, Hook, spsc>
  : public concurrent_queue<T, Hook, mpmc>
{
};


__sal_end
} // namespace sal
