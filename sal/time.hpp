#pragma once

/**
 * \file sal/time.hpp
 * Portability wrapper for ctime functions
 */

#include <sal/config.hpp>
#include <chrono>
#include <ctime>


__sal_begin


/// System clock
using clock_t = std::chrono::system_clock;

/// System clock timestamp
using time_t = clock_t::time_point;


/// Return system clock's current time
inline time_t now () noexcept
{
  return clock_t::now();
}


/**
 * Portability wrapper for localtime(). This function's API differs from
 * standard and uses internally platform-specific function.
 *
 * \returns Broken down std::tm structure for \a time
 */
inline std::tm local_time (const std::time_t &time) noexcept
{
  std::tm tm;
#if __sal_os_windows
  ::localtime_s(&tm, &time);
#else
  ::localtime_r(&time, &tm);
#endif
  return tm;
}


/**
 * Portability wrapper for localtime(). This function's API differs from
 * standard and uses internally platform-specific function.
 *
 * \returns Broken down std::tm structure for \a time
 */
inline std::tm local_time (const time_t &time) noexcept
{
  return local_time(clock_t::to_time_t(time));
}


/**
 * Portability wrapper for localtime(). This function's API differs from
 * standard and uses internally platform-specific function.
 *
 * \returns Broken down std::tm structure for current moment.
 */
inline std::tm local_time () noexcept
{
  return local_time(now());
}


/**
 * Portability wrapper for gmtime(). This function's API differs from
 * standard and uses internally platform-specific function.
 *
 * \returns Broken down std::tm structure for \a time
 */
inline std::tm utc_time (const std::time_t &time) noexcept
{
  std::tm tm;
#if __sal_os_windows
  ::gmtime_s(&tm, &time);
#else
  ::gmtime_r(&time, &tm);
#endif
  return tm;
}


/**
 * Portability wrapper for gmtime(). This function's API differs from
 * standard and uses internally platform-specific function.
 *
 * \returns Broken down std::tm structure for \a time
 */
inline std::tm utc_time (const time_t &time) noexcept
{
  return utc_time(clock_t::to_time_t(time));
}


/**
 * Portability wrapper for gmtime(). This function's API differs from
 * standard and uses internally platform-specific function.
 *
 * \returns Broken down std::tm structure for current moment.
 */
inline std::tm utc_time () noexcept
{
  return utc_time(now());
}


/**
 * Return offset between UTC and local time at \a time. This function takes
 * into account timezone and possible daylight savings.
 */
inline std::chrono::seconds local_offset (const time_t &time) noexcept
{
  std::tm local = local_time(time);
#if __sal_os_windows
  std::tm utc = utc_time(time);
  utc.tm_isdst = local.tm_isdst;
  return std::chrono::seconds(clock_t::to_time_t(time) - mktime(&utc));
#else
  return std::chrono::seconds(local.tm_gmtoff);
#endif
}


__sal_end
