#pragma once

/**
 * \file sal/logger/fwd.hpp
 */

#include <memory>


namespace sal { namespace logger {
__sal_begin


// sal/logger/level.hpp
enum class level_t: uint8_t;
class threshold_t;

// sal/logger/event.hpp
struct event_t;
using event_ptr = std::unique_ptr<event_t, void(*)(event_t *)>;

// sal/logger/logger.hpp
template <typename Worker> class logger_t;
template <typename Worker> class basic_worker_t;

// sal/logger/sink.hpp
class sink_base_t;
using sink_ptr = std::shared_ptr<sink_base_t>;


__sal_end
}} // namespace sal::logger
