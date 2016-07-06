#pragma once

#include <sal/logger/level.hpp>
#include <sal/common.test.hpp>
#include <ostream>


namespace sal_test {


static auto logger_levels = testing::Values(
  sal::logger::level_t::ERROR,
  sal::logger::level_t::WARN,
  sal::logger::level_t::INFO,
  sal::logger::level_t::DEBUG
);


inline sal::logger::level_t more_verbose (sal::logger::level_t level) noexcept
{
  return static_cast<sal::logger::level_t>(
    static_cast<int>(level) + 1
  );
}


inline sal::logger::level_t less_verbose (sal::logger::level_t level) noexcept
{
  return static_cast<sal::logger::level_t>(
    static_cast<int>(level) - 1
  );
}


} // namespace sal_test


namespace sal { namespace logger {


inline std::ostream &operator<< (std::ostream &os, sal::logger::level_t level)
{
  static const char *level_names[] = { "OFF", "ERROR", "WARN", "INFO", "DEBUG" };
  os << level_names[static_cast<int>(level)];
  return os;
}


}} // namespace sal::logger
