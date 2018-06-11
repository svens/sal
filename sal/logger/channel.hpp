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
 * from worker, it'll receive reference to it that remains valid until worker
 * object itself is destructed.
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


__sal_begin


namespace logger {


/**
 * Main event logging API. It provides only limited methods to check if
 * logging is enabled and event factory method. This API is intentionally
 * limited to separate channel creation and configuring from logging events.
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
  bool is_logger_channel_enabled () const noexcept
  {
    return impl_.is_enabled;
  }


  /**
   * Set \a enabled state for this channel.
   */
  void set_enabled (bool enabled) noexcept
  {
    impl_.is_enabled = enabled;
  }


  /**
   * Create and return new logging event.
   *
   * \note Event creation is unconditional, even if is_enabled() returns
   * false. Use logging macros that do this checking.
   */
  event_ptr make_logger_event () const
  {
    return impl_.worker.make_logger_event(*this);
  }


private:

  using impl_t = __bits::channel_t<Worker>;
  impl_t &impl_;

  channel_t (impl_t &impl)
    : impl_(impl)
  {}

  friend class basic_worker_t<Worker>;
  friend Worker;
};


} // namespace logger


__sal_end

/// \}
