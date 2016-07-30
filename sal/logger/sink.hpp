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
#include <sal/logger/event.hpp>


namespace sal { namespace logger {
__sal_begin


/**
 * Base class for all sinks.
 *
 * Default implementation formats message prefix as:
 * \code{.txt}
 * HH:MM:SS,MSEC\tTHREAD\t[channel]
 * \endcode
 * where:
 *   - HH:MM:SS,MSEC: current timestamp in GMT
 *   - THREAD: logging thread id
 *   - [channel]: channel name (if empty, then not printed)
 *
 * Inherited sink could override method(s):
 *   - sink_event_init(): do initial event formatting. Called after event is
 *     created. Default implementation formats message as described above,
 *     using local time.
 *   - sink_event_write(): do final event formatting and write message to
 *     destination. There is no default implementation (pure virtual method)
 *
 * Thread-safety: sinks can be shared between multiple channels and/or workers
 * with following rules:
 *  - sink_event_init() is called from any application thread context that
 *    sends event to channel i.e. if it has side-effects in sink itself, it is
 *    implementation responsibility to handle synchronisation
 *  - sink_event_write() is called in worker thread context only i.e. no
 *    synchronisation is necessary. But, if sink is shared between multiple
 *    workers (not recommended), then it might be called from multiple
 *    threads, in which case is implementation reponsibility to handle
 *    synchronisation
 */
class sink_t
{
public:

  virtual ~sink_t () = default;


  /**
   * Initialise logging \a event message. This method is called immediately
   * after channel has created \a event. New implementation can do initial
   * formatting (date/time, channel name, etc).
   */
  virtual void sink_event_init (event_t &event,
    const std::string &channel_name)
  {
    event.time = local_now();
    init(event, channel_name);
  }


  /**
   * Write \a event message to actual implementation-specific destination. New
   * implementation can also do final formatting if necessary.
   */
  virtual void sink_event_write (event_t &event) = 0;


protected:

  static time_t local_now () noexcept;
  void init (event_t &event, const std::string &channel_name) noexcept;
};


/**
 * Return sink that prints event messages into std::cout, using default layout
 * implemented by sink_t.
 */
sink_ptr cout_sink ();


/**
 * Return sink that prints event messages into std::cerr, using default layout
 * implemented by sink_t.
 */
sink_ptr cerr_sink ();


__sal_end
}} // namespace sal::logger

/// \}
