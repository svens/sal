#pragma once

// SingleProducer / MultiConsumer queue implementation
//
// included by sal/atomic_queue.hpp with necessary types already provided
// here we specialise atomic_queue<> for spmc


namespace sal {
__sal_begin


template <typename T, atomic_queue_hook<T> T::*Hook>
class atomic_queue<T, Hook, spmc>
  : public atomic_queue<T, Hook, mpmc>
{
};


__sal_end
} // namespace sal
