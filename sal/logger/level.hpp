#pragma once

/**
 * \file sal/logger/level.hpp
 * Logging event verbosity level
 */


#include <sal/config.hpp>


namespace sal { namespace logger {
__sal_begin


/// Turn off logging
#define SAL_LOGGER_LEVEL_OFF 0
/// Error level event
#define SAL_LOGGER_LEVEL_ERROR 1
/// Warning level event
#define SAL_LOGGER_LEVEL_WARN 2
/// Information level event
#define SAL_LOGGER_LEVEL_INFO 3
/// Debug level event
#define SAL_LOGGER_LEVEL_DEBUG 4


/// Logger event verbosity level
enum class level_t: uint8_t
{
  ERROR = SAL_LOGGER_LEVEL_ERROR,
  WARN = SAL_LOGGER_LEVEL_WARN,
  INFO = SAL_LOGGER_LEVEL_INFO,
  DEBUG = SAL_LOGGER_LEVEL_DEBUG,
};


/**
 * Logging verbosity threshold. Logging level values that are numerically
 * greater than threshold are disabled.
 */
class threshold_t
{
public:

  threshold_t () = default;


  /// Create threshold with \a level
  constexpr threshold_t (level_t level) noexcept
    : level_(level)
  {}


  /// Return true if \a level is enabled due threshold
  constexpr bool is_enabled (level_t level) const noexcept
  {
    return level_ >= level;
  }


private:

  level_t level_ = level_t::INFO;
};


/**
 * Check if \a level exceeds externally defined SAL_LOGGER_THRESHOLD.
 *
 * This allows compile-time logging threshold dependent statements that are
 * optimized away if \a level is below threshold. Usage:
 * \code
 * if (sal::logger::is_enabled(sal::logger::level_t::INFO))
 * {
 *   // executed only if SAL_LOGGER_THRESHOLD is undefined, or
 *   // defined as SAL_LOGGER_LEVEL_INFO or higher
 * }
 * \endcode
 *
 * \note Value for SAL_LOGGER_THRESHOLD must be defined before including this
 * header. Possible values are:
 *  - SAL_LOGGER_LEVEL_OFF
 *  - SAL_LOGGER_LEVEL_ERROR
 *  - SAL_LOGGER_LEVEL_WARN
 *  - SAL_LOGGER_LEVEL_INFO
 *  - SAL_LOGGER_LEVEL_DEBUG
 *
 * \note Setting SAL_LOGGER_THRESHOLD in compilation unit affects only given
 * unit. To change threshold for whole project, specify it's value in
 * buildsystem for all sources.
 *
 * \see SAL_LOGGER_ENABLED(level)
 */
constexpr bool is_enabled (level_t level) noexcept
{
#if defined(SAL_LOGGER_THRESHOLD)
  return static_cast<level_t>(SAL_LOGGER_THRESHOLD) >= level;
#else
  return static_cast<int>(level) != SAL_LOGGER_LEVEL_OFF;
#endif
}


/**
 * Macro wrapper for sal::logger::is_enabled()
 *
 * Possible \a level values are:
 *  - ERROR
 *  - WARN
 *  - INFO
 *  - DEBUG
 */
#define SAL_LOGGER_ENABLED(level) \
  (sal::logger::is_enabled(sal::logger::level_t::level))


__sal_end
}} // namespace sal::logger
