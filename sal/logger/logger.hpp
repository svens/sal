#pragma once

/**
 * \file sal/logger/logger.hpp
 * Main logger API
 *
 * API is divided into two connected parts:
 *   - sal::logger::logger_t: logging API
 *   - Worker (different implementations): maintains list of loggers,
 *     their configuration and dispatches logging events to final
 *     destination(s).
 *
 * Each logger is owned by worker. When application layer queries logger_t
 * from worker, it'll receive const reference to it that remains valid until
 * worker object itself is destructed.
 *
 * Recommended usage at application layer:
 *   - during startup, add and configure all necessary loggers
 *   - pass worker (loggers' owner) around, so application modules can fetch
 *     reference to their logger and use it during runtime. It is not
 *     recommended to lookup for logger on every event logging (it does
 *     internally std::unordered_map lookup)
 *
 * \addtogroup logger
 * \{
 */


#include <sal/config.hpp>
#include <sal/logger/__bits/logger.hpp>
#include <sal/logger/fwd.hpp>


namespace sal { namespace logger {
__sal_begin


/**
 * Main logger API. It provides only const methods to check if logging is
 * enabled and event factory method. This API is intentionally limited to
 * separate logger creation and configuring from logging events.
 *
 * \a Worker owns logger_t instances and manages their lifecycle. Possible
 * implementations provided by this library are worker_t (synchronous logging)
 * and async_worker_t (asynchronous logging). This class has internally only
 * reference to actual logger. This way it is cheap to copy and pass around.
 *
 * \see worker_t
 * \see async_worker_t
 *
 * \note It is not recommended to use this class' methods directly but through
 * logging macros that can be configured compile time.
 */
template <typename Worker>
class logger_t
{
public:

  /**
   * Return logger's name (i.e. module name that logs using this logger)
   */
  const std::string &name () const noexcept
  {
    return impl_.name;
  }


  /**
   * Return true if logging events at \a level are enabled i.e. exceed
   * configured threshold.
   */
  bool is_enabled (level_t level) const noexcept
  {
    return impl_.threshold.is_enabled(level);
  }


  /**
   * Create and return new logging event.
   *
   * \note Event creation is unconditional, even if is_enabled(\a level)
   * returns false. Use logging macros that do this checking
   */
  event_ptr make_event (level_t level) const
  {
    return event_ptr{
      impl_.worker.alloc_and_init(level, *this),
      &Worker::write_and_release
    };
  }


private:

  using impl_t = __bits::logger_t<Worker>;
  const impl_t &impl_;

  logger_t (const impl_t &impl)
    : impl_(impl)
  {}

  friend class basic_worker_t<Worker>;
  friend Worker;
};


/**
 * \def sal_log(dest,level)
 * Function-like macro to ease logging
 *
 * Usage:
 * \code
 * sal_log(logger, sal::logger::level_t::INFO) << "result=" << slow_call();
 * \endcode
 *
 * If logging is disabled (either compile- or runtime), statement is optimized
 * away as much as possible:
 *   - compile time disabling: statement renders completely to nothing with
 *     any decent compiler that is able to optimise away 'if (false)'
 *     statement. See sal::logger::is_enabled() for more information about how
 *     to disable logging at compile time.
 *   - runtime disabling: statement is rendered to if-statement that checks if
 *     logger has level disabled. In else branch, unnamed event_ptr object is
 *     instantiated for logging. Event message is logged when going out of
 *     scope.
 *
 * \warning Beware of side-effects of disabled logging, all the possible calls
 * are optimised away. Required calls should be separate statements outside
 * logging:
 * \code
 * // on disable logging, function is not invoked
 * sal_log(logger, sal::logger::level_t::INFO) << save_the_world();
 *
 * // function is invoked regardless whether logging is disabled
 * auto result = save_the_world();
 * sal_log(logger, sal::logger::level_t::INFO) << result;
 * \endcode
 */
#define sal_log(dest,level) \
  if (!sal::logger::is_enabled(level)) /**/; \
  else if (!(dest).is_enabled(level)) /**/; \
  else (dest).make_event(level)->message


/// \copybrief sal_log at error level
#define sal_log_error(dest) sal_log(dest,sal::logger::level_t::ERROR)
/// \copybrief sal_log at warning level
#define sal_log_warn(dest) sal_log(dest,sal::logger::level_t::WARN)
/// \copybrief sal_log at info level
#define sal_log_info(dest) sal_log(dest,sal::logger::level_t::INFO)
/// \copybrief sal_log at debug level
#define sal_log_debug(dest) sal_log(dest,sal::logger::level_t::DEBUG)


/// \copybrief sal_log at error level to sal::logger::default_logger()
#define sal_error sal_log_error(sal::logger::default_logger())
/// \copybrief sal_log at warning level to sal::logger::default_logger()
#define sal_warn sal_log_warn(sal::logger::default_logger())
/// \copybrief sal_log at info level to sal::logger::default_logger()
#define sal_info sal_log_info(sal::logger::default_logger())
/// \copybrief sal_log at debug level to sal::logger::default_logger()
#define sal_debug sal_log_debug(sal::logger::default_logger())


/**
 * Log message only if \a expr is true. If logging is disabled at \a level for
 * \a dest (either compile- or runtime), this call is no-op.
 *
 * Usage:
 * \code
 * sal_log_if(logger, sal::logger::level_t:::INFO, x > y) << "X is bigger than Y";
 * \endcode
 *
 * \note \a expr and message inserter calls are evaluated only if logging is
 * enabled.
 */
#define sal_log_if(dest,level,expr) \
  if (!sal::logger::is_enabled(level)) /**/; \
  else if (!(dest).is_enabled(level)) /**/; \
  else if (!(expr)) /**/; \
  else (dest).make_event(level)->message


/// \copybrief sal_log_if at error level
#define sal_log_error_if(dest,expr) sal_log_if(dest,sal::logger::level_t::ERROR,expr)
/// \copybrief sal_log_if at warning level
#define sal_log_warn_if(dest,expr) sal_log_if(dest,sal::logger::level_t::WARN,expr)
/// \copybrief sal_log_if at info level
#define sal_log_info_if(dest,expr) sal_log_if(dest,sal::logger::level_t::INFO,expr)
/// \copybrief sal_log_if at debug level
#define sal_log_debug_if(dest,expr) sal_log_if(dest,sal::logger::level_t::DEBUG,expr)


/// \copybrief sal_log_if at error level to sal::logger::default_logger()
#define sal_error_if(expr) sal_log_error_if(sal::logger::default_logger(),expr)
/// \copybrief sal_log_if at warn level to sal::logger::default_logger()
#define sal_warn_if(expr) sal_log_warn_if(sal::logger::default_logger(),expr)
/// \copybrief sal_log_if at info level to sal::logger::default_logger()
#define sal_info_if(expr) sal_log_info_if(sal::logger::default_logger(),expr)
/// \copybrief sal_log_if at debug level to sal::logger::default_logger()
#define sal_debug_if(expr) sal_log_debug_if(sal::logger::default_logger(),expr)


__sal_end
}} // namespace sal::logger

// \}
