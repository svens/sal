#pragma once

/**
 * \file sal/logger/event.hpp
 * Logging event information
 *
 * \addtogroup logger
 * \{
 */

#include <sal/config.hpp>
#include <sal/logger/fwd.hpp>
#include <sal/str.hpp>


namespace sal { namespace logger {
__sal_begin


/**
 * Logging event. This class shouldn't be instantiated directly but through
 * channel that handles event lifecycle etc.
 */
struct event_t
{
  /// Maximum message length
  static constexpr size_t max_message_size = 4000
    - sizeof(sink_t *)
    - sizeof(void *)
    - sizeof(str_t<1>) - 1
  ;

  /// Final sink where event will be sent to
  sink_t *sink{};

  /// Opaque sink-specific data (should be used only by sink)
  void *sink_data{};

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

/// \}
