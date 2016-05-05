#pragma once

/**
 * \file sal/concurrent_queue.hpp
 * Synchronised concurrent queues (FIFO)
 *
 * This module provides common API for multiple open-ended queue
 * implementations with different traits.
 */

#include <sal/config.hpp>
#include <type_traits>
#include <utility>


namespace sal {
__sal_begin


/// concurrent_queue usage policy
template <bool MultiProducer, bool MultiConsumer>
struct concurrent_queue_use_policy
{
  /// If true, concurrent_queue::push() can be used from multiple threads
  static constexpr bool multi_producer = MultiProducer;

  /// If true, concurrent_queue::try_pop() can be used from multiple threads
  static constexpr bool multi_consumer = MultiConsumer;
};


/// Single producer/consumer queue type
using spsc = concurrent_queue_use_policy<false, false>;

/// Multiple producers, single consumer queue type
using mpsc = concurrent_queue_use_policy<true, false>;

/// Single producer, multiple consumers queue type
using spmc = concurrent_queue_use_policy<false, true>;

/// Multiple producers/consumers queue type
using mpmc = concurrent_queue_use_policy<true, true>;


/**
 * Synchronised concurrent queue (FIFO).
 *
 * \a UsePolicy dictates how many producers and consumers can use queue
 * concurrently. Even if MPMC is most generic that can fulfill requirements of
 * other use policies, it's performance might be less scalable. It is
 * recommended to choose correct \a UsePolicy for specific situations.
 *
 * Memory management is left responsibility of application. Although these
 * queues are not intrusive, they expect application to provide nodes that
 * contain queue-specific maintenance fields and also data field for
 * application own usage. Queue-specific node fields are left opaque (they
 * might be different for different UsePolicy queues). These fields and node's
 * data field are disjointed i.e. pushing node n1 with data d1 and later
 * popping d1 might return it inside different node. Queue implementation
 * moves one node's data to another when popping node. For this reason, to
 * improve performance, data field should be either trivial type (cheaply
 * copied) or movable.
 *
 * During queue construction, it expects stub node for internal queue
 * bookkeeping. Later popping might return this as filled node and use
 * another pushed node as new stub. From application perspective, stub should
 * be handled similarly to other nodes. When queue is empty, stub is still
 * kept internally. Management of stub node's data is left to application.
 *
 * Because memore management is application's responsibility, destructor does
 * nothing i.e. application should either pop and free all items from queue if
 * allocated from heap or if uses stack-based nodes, can ignore this issue as
 * well. In heap-based nodes' case, application can query remaining stub node
 * before destructing queue. Using it in any other situation is undefined
 * behaviour.
 *
 * In summary, everything is application's responsibility, concurrent_queue
 * just provides queueing functionality and requires application to provide
 * nodes with \a T that can be moved between nodes.
 *
 * Sample queue usage with stack-based nodes:
 * \code
 * using queue = sal::concurrent_queue<int, sal::spsc>;
 * using node = typename queue::node;
 *
 * node stub, n1{1}, n2{2};
 * queue q(&stub);
 * q.push(&n1);
 * q.push(&n2);
 *
 * auto n = q.try_pop();
 * assert(n->data == 1);
 *
 * n = q.try_pop();
 * assert(n->data == 2);
 *
 * n = q.try_pop();
 * assert(n == nullptr);
 * \endcode
 *
 * Sample queue usage with heap-allocated nodes:
 * \code
 * using queue = sal::concurrent_queue<int, sal::spsc>;
 * using node = typename queue::node;
 *
 * queue q(new node);
 * q.push(new node{1});
 * q.push(new node{2});
 *
 * std::unique_ptr<node> n(q.try_pop());
 * assert(n.get()->data == 1);
 *
 * n.reset(q.try_pop());
 * assert(n.get()->data == 2);
 *
 * n.reset(q.try_pop());
 * assert(n == nullptr);
 *
 * // n dtor takes care of stub when q & n go out of scope
 * n.reset(q.stub());
 * \endcode
 */
template <typename T, typename UsePolicy>
class concurrent_queue
{
public:

  /**
   * Queue use policy
   * \see concurrent_queue_use_policy
   */
  using use_policy = UsePolicy;


  /// Queue node
  struct node
  {
    /// Application-specific data
    T data{};

    /// Construct new node with copy of application \a data
    node (const T &data)
      : data(data)
    {}

    /// Construct new node with application data constructed using \a args
    template <typename... Args>
    node (Args &&...args)
      : data(std::forward<Args>(args)...)
    {}
  };


  concurrent_queue () = delete;
  concurrent_queue (const concurrent_queue &) = delete;
  concurrent_queue &operator= (const concurrent_queue &) = delete;


  /// Create new queue using \a stub
  concurrent_queue (node *stub) noexcept;


  /**
   * Move \a that to \a this. Using \a that after move is undefined behaviour.
   * Move construction is not synchronised i.e. application should avoid
   * accessing \a that during and after move.
   */
  concurrent_queue (concurrent_queue &&that) noexcept;


  /**
   * Move all nodes from \a that to \a this. Using \a that after move is
   * undefined behaviour.  Move assignment is not synchronised i.e.
   * application should avoid accessing \a this and \a that during and after
   * move.
   *
   * Existing items in \a this are simply discarded i.e. it is application
   * responsibility to empty queue if nodes are allocated from heap.
   */
  concurrent_queue &operator= (concurrent_queue &&that) noexcept;


  /// Return true if queue uses no locks during concurrent access
  bool is_lock_free () const noexcept;


  /**
   * Return pointer to internal stub node. Application should use this getter
   * only just before destructing queue (i.e. to release stub node if
   * allocated from heap). Application can destruct stub either before or
   * after destructing queue because queue itself does not touch possible
   * nodes during this.
   */
  node *stub () const noexcept;


  /// Push new node \a n into queue
  void push (node *n) noexcept;


  /**
   * Pop and return next node from queue or nullptr if queue is empty
   * \note Even if queue is empty, it still holds stub.
   */
  node *try_pop () noexcept(std::is_nothrow_move_assignable<T>::value);
};


__sal_end
} // namespace sal


// specializations for different producer/consumer policies
#include <sal/__bits/concurrent_queue_mpmc.hpp>
#include <sal/__bits/concurrent_queue_mpsc.hpp>
#include <sal/__bits/concurrent_queue_spmc.hpp>
#include <sal/__bits/concurrent_queue_spsc.hpp>
