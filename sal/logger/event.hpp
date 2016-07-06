#pragma once

/**
 * \file sal/logger/event.hpp
 * Logging event
 */


#include <sal/config.hpp>
#include <sal/logger/fwd.hpp>
#include <sal/str.hpp>
#include <sal/time.hpp>
#include <string>


namespace sal { namespace logger {
__sal_begin


struct event_t
{
  static constexpr size_t max_message_size = 4000
    - sizeof(str_t<1>) - 1
    - sizeof(level_t)
    - sizeof(time_t)
    - sizeof(sink_base_t *)
    - sizeof(const std::string *)
  ;

  str_t<max_message_size> message;
  level_t level;
  time_t time;
  sink_base_t *sink;
  const std::string *logger_name;
};


__sal_end
}} // namespace sal::logger
