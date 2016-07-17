#pragma once

/**
 * \file sal/logger/channel.hpp
 * Main channel API
 *
 * API is divided into two connected parts:
 *   - sal::logger::channel_t: event logging API
 *   - Worker (different implementations): maintains list of channels,
 *     their configuration and dispatches logging events to final
 *     destination(s).
 *
 * Each channel is owned by worker. When application layer queries channel_t
 * from worker, it'll receive const reference to it that remains valid until
 * worker object itself is destructed.
 *
 * Recommended usage at application layer:
 *   - during startup, add and configure all necessary channels
 *   - pass worker (channels' owner) around, so application modules can fetch
 *     reference to their channel and use it during runtime. It is not
 *     recommended to lookup for channel on every event logging (it does
 *     internally std::unordered_map lookup)
 *
 * \addtogroup logger
 * \{
 */


#include <sal/config.hpp>
#include <sal/logger/__bits/channel.hpp>


namespace sal { namespace logger {
__sal_begin


/**
 * Main event logging API. It provides only const methods to check if logging
 * is enabled and event factory method. This API is intentionally limited to
 * separate channel creation and configuring from logging events.
 *
 * \a Worker owns channel_t instances and manages their lifecycle. Possible
 * implementations provided by this library are worker_t (synchronous logging)
 * and async_worker_t (asynchronous logging). This class has internally only
 * reference to actual channel. This way it is cheap to copy and pass around.
 *
 * \see worker_t
 * \see async_worker_t
 *
 * \note It is not recommended to use this class' methods directly but through
 * logging macros that can be configured compile time.
 */
template <typename Worker>
class channel_t
{
public:

  /**
   * Return channel's name (e.g. module name that logs using this channel)
   */
  const std::string &name () const noexcept
  {
    return impl_.name;
  }


  /**
   * Return true if logging events to this channel are enabled.
   */
  bool is_enabled () const noexcept
  {
    return impl_.is_enabled;
  }


  /**
   * Create and return new logging event.
   *
   * \note Event creation is unconditional, even if is_enabled() returns
   * false. Use logging macros that do this checking.
   */
  event_ptr make_event () const
  {
    return event_ptr{
      impl_.worker.alloc_and_init(*this),
      &Worker::write_and_release
    };
  }


private:

  using impl_t = __bits::channel_t<Worker>;
  const impl_t &impl_;

  channel_t (const impl_t &impl)
    : impl_(impl)
  {}

  friend class basic_worker_t<Worker>;
  friend Worker;
};


/**
 * \def sal_log(channel)
 * Function-like macro to ease logging
 *
 * Usage:
 * \code
 * sal_log(channel) << "result=" << slow_call();
 * \endcode
 *
 * In case of disabled logging, statement is rendered to channel.is_enabled()
 * call. In else branch, unnamed event_ptr object is instantiated for logging.
 * Event message is logged when going out of scope.
 *
 * \warning Beware of side-effects of disabled logging, all the possible calls
 * are optimised away. Required calls should be separate statements outside
 * logging:
 * \code
 * // on disable logging, function is not invoked
 * sal_log(channel) << save_the_world();
 *
 * // function is invoked regardless whether logging is disabled
 * auto result = save_the_world();
 * sal_log(channel) << result;
 * \endcode
 */
#define sal_log(channel) \
  if (!(channel).is_enabled()) /**/; \
  else (channel).make_event()->message


/**
 * Log message only if \a expr is true. If logging is disabled for \a channel,
 * this call is no-op.
 *
 * Usage:
 * \code
 * sal_log_if(channel, x > y) << "X is bigger than Y";
 * \endcode
 *
 * \note \a expr and message inserter calls are evaluated only if logging is
 * enabled.
 */
#define sal_log_if(channel,expr) \
  if (!(channel).is_enabled()) /**/; \
  else if (!(expr)) /**/; \
  else (channel).make_event()->message


__sal_end
}} // namespace sal::logger

/// \}
