#pragma once

/**
 * \file sal/logger/async_worker.hpp
 * Asynchronous worker, marshalling event records from channels to sinks.
 *
 * \addtogroup logger
 * \{
 */

#include <sal/config.hpp>
#include <sal/logger/worker.hpp>
#include <memory>


__sal_begin


namespace logger {


/**
 * Asynchronous logger worker. It uses separate thread to write event records
 * to final destinations asynchronously. Events are sent from logging threads
 * to worker thread using queues. Event records are reused: after writing
 * event, it is not released but stored in free list. Next logging will
 * acquire event record from there or allocates new one if no free event at
 * that moment.
 *
 * Compared to worker_t, asynchronous worker does block logging thread for
 * shorter period (possible event record allocation when there is no free
 * records in pool). When specific application does not need such non-blocking
 * behavior, use worker_t instead as it is simpler and does not do allocations
 * on it's own.
 */
class async_worker_t
  : public basic_worker_t<async_worker_t>
{
public:

  /**
   * Construct new asynchronous worker for logging.
   */
  template <typename... Options>
  async_worker_t (Options &&...options)
    : basic_worker_t(std::forward<Options>(options)...)
  {}


private:

  struct impl_t;
  using impl_ptr = std::unique_ptr<impl_t, void(*)(impl_t *)>;

  impl_ptr start ();
  impl_ptr impl_ = start();

  event_ptr make_event (const channel_type &channel) noexcept;
  friend class channel_t<async_worker_t>;
};


} // namespace logger


__sal_end

/// \}
