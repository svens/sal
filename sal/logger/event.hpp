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


struct event_t
{
  static constexpr size_t max_message_size = 4000
    - sizeof(level_t)
    - sizeof(time_t)
    - sizeof(thread_id)
    - sizeof(str_t<1>) - 1
    - sizeof(const std::string *)
    - sizeof(sink_base_t *)
  ;

  level_t level{};
  time_t time{};
  thread_id thread{};
  str_t<max_message_size> message{};
  const std::string *logger_name{};
  sink_base_t *sink{};
};


__sal_end
}} // namespace sal::logger
