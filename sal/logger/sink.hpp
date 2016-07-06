#pragma once

/**
 * \file sal/logger/sink.hpp
 * Logging events sink.
 */


#include <sal/config.hpp>
#include <sal/logger/fwd.hpp>


namespace sal { namespace logger {
__sal_begin


/**
 * Base class for all sinks. Actual sink should implement at least
 * event_write() that takes care of final formatting and writing logging event
 * to destination. Optionally event_init() can be also implemented which is
 * called by logger immediately after new event is created. There initial
 * formatting can be done, etc. Default implementation does nothing.
 *
 * Thread-safety: sinks can be shared between multiple loggers with following
 * rules:
 *  - init() is called from any application thread context that sends event to
 *    logger i.e. if it has side-effects in sink itself, it is implementation
 *    responsibility to handle synchronisation
 *  - write() is called in logger thread context only i.e. no synchronisation
 *    is necessary. But, if sink is shared between multiple logger list
 *    objects (not recommended), then it might be called from multiple
 *    threads, in which case is implementation reponsibility to handle
 *    synchronisation
 */
class sink_base_t
{
public:

  virtual ~sink_base_t () = default;


  /**
   * Invoke virtual event_init() to initialise logger \a event. This method
   * is called immediately after logger has created \a event. New
   * implementation can do initial formatting (date/time, logger name, etc).
   *
   * Default implementation does nothing.
   */
  void init (event_t &event)
  {
    event_init(event);
  }


  /**
   * Invoke virtual event_write() to write \a event message to actual
   * implementation-specific destination. New implementation can also do final
   * formatting (adding suffixes, etc).
   */
  void write (event_t &event)
  {
    event_write(event);
  }


protected:

  virtual void event_init (event_t &event)
  {
    (void)event;
  }

  virtual void event_write (event_t &event) = 0;
};


__sal_end
}} // namespace sal::logger
