#pragma once

/**
 * \file sal/logger/fwd.hpp
 * Forward declarations
 *
 * \addtogroup logger
 * \{
 */

#include <memory>


__sal_begin


/// Logging
namespace logger {


// sal/logger/event.hpp
struct event_t;

/**
 * Pointer to current event logging data
 */
using event_ptr = std::unique_ptr<event_t, void(*)(event_t *)>;

// sal/logger/sink.hpp
class sink_t;

/**
 * Pointer to sink where event logging data is sent.
 */
using sink_ptr = std::shared_ptr<sink_t>;

// sal/logger/channel.hpp
template <typename Worker> class channel_t;

// sal/logger/worker.hpp
template <typename Worker> class basic_worker_t;
class worker_t;

// sal/logger/async_worker.hpp
class async_worker_t;


} // namespace logger


__sal_end

/// \}
