#pragma once

/**
 * \file sal/logger/logger.hpp
 * Macros for more convenient logging
 *
 * \addtogroup logger
 * \{
 */


#include <sal/config.hpp>
#include <sal/logger/channel.hpp>
#include <sal/logger/worker.hpp>


namespace sal { namespace logger {
__sal_begin


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
 * // on disabled logging, function is not invoked
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


/**
 * \def sal_print
 * Wrapper for sal_log(channel) using sal::logger::default_channel() as
 * channel.
 */
#define sal_print sal_log(sal::logger::default_channel())


/**
 * \def sal_print_if(expr)
 * Wrapper for sal_log_if(channel,expr) using sal::logger::default_channel()
 * as channel.
 */
#define sal_print_if(expr) sal_log_if(sal::logger::default_channel(), (expr))


__sal_end
}} // namespace sal::logger

/// \}
