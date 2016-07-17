#pragma once

/**
 * \file sal/logger/sink.hpp
 * Logging events sink.
 *
 * \addtogroup logger
 * \{
 */


#include <sal/config.hpp>
#include <sal/logger/fwd.hpp>


namespace sal { namespace logger {
__sal_begin


/**
 * Base class for all sinks. Default implementation formats message prefix as:
 * \code{.txt}
 * HH:MM:SS,MSEC\tTHREAD\t[channel]
 * \endcode
 * where:
 *   - HH:MM:SS,MSEC: current timestamp in GMT
 *   - THREAD: logging thread id
 *   - [channel]: channel name (if empty, then not printed)
 * and prints message using printf().
 *
 * Inherited sink could override methods:
 *   - event_init(): do initial formatting. Called after event is created
 *   - event_write(): do final formatting and write message to destination
 *
 * Thread-safety: sinks can be shared between multiple channels and/or workers
 * with following rules:
 *  - init() is called from any application thread context that sends event to
 *    channel i.e. if it has side-effects in sink itself, it is implementation
 *    responsibility to handle synchronisation
 *  - write() is called in worker thread context only i.e. no synchronisation
 *    is necessary. But, if sink is shared between multiple workers (not
 *    recommended), then it might be called from multiple threads, in which
 *    case is implementation reponsibility to handle synchronisation
 */
class sink_t
{
public:

  virtual ~sink_t () = default;


  /**
   * Invoke virtual event_init() to initialise logging \a event message. This
   * method is called immediately after channel has created \a event. New
   * implementation can do initial formatting (date/time, channel name, etc).
   *
   * \note event fields are already initialised (and message is empty)
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

  /// \see init
  virtual void event_init (event_t &event);

  /// \see write
  virtual void event_write (event_t &event);
};


__sal_end
}} // namespace sal::logger

/// \}
