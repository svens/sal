#pragma once

/**
 * \file sal/logger/event.hpp
 * Logging event
 */


#include <sal/config.hpp>
#include <sal/logger/fwd.hpp>
#include <sal/str.hpp>
#include <sal/thread.hpp>
#include <sal/time.hpp>
#include <string>


namespace sal { namespace logger {
__sal_begin


/**
 * Logging event. This class shouldn't be instantiated directly but through
 * logger that handles event lifecycle etc.
 */
struct event_t
{
  /// Maximum message length
  static constexpr size_t max_message_size = 4000
    - sizeof(level_t)
    - sizeof(time_t)
    - sizeof(thread_id)
    - sizeof(sink_t *)
    - sizeof(const std::string *)
    - sizeof(str_t<1>) - 1
  ;

  /// Event level
  level_t level{};

  /// Event time
  time_t time{};

  /// Event logging thread
  thread_id thread{};

  /// Final sink where event will be sent to
  sink_t *sink{};

  /// Name of logger that created this event
  const std::string *logger_name{};

  /// Event message
  str_t<max_message_size> message{};


private:

  static void static_checks () noexcept
  {
    static_assert(sizeof(event_t) <= 4096, "fit event_t into 4kB");
  }
};


__sal_end
}} // namespace sal::logger
