#pragma once

#include <sal/logger/level.hpp>
#include <sal/logger/sink.hpp>
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


struct sink_t final
  : public sal::logger::sink_t
{
  bool init_called, write_called;
  bool throw_init, throw_write;
  std::string last_message;
  sal::logger::level_t last_level;

  sink_t ()
  {
    reset();
  }

  void reset ()
  {
    init_called = write_called = throw_init = throw_write = false;
    last_level = static_cast<sal::logger::level_t>(SAL_LOGGER_LEVEL_OFF);
    last_message = "";
  }

  void event_init (sal::logger::event_t &event) override
  {
    sal::logger::sink_t::event_init(event);

    init_called = true;
    if (throw_init)
    {
      throw_init = false;
      throw false;
    }
  }

  void event_write (sal::logger::event_t &event) override
  {
    write_called = true;
    if (throw_write)
    {
      throw_write = false;
      throw false;
    }
    last_message = sal::to_string(event.message);
    last_level = event.level;
  }

  bool last_message_contains (const std::string &value)
  {
    return last_message.find(value) != last_message.npos;
  }
};


} // namespace sal_test


namespace sal { namespace logger {


inline std::ostream &operator<< (std::ostream &os, sal::logger::level_t level)
{
  static const char *level_names[] = { "OFF", "ERROR", "WARN", "INFO", "DEBUG" };
  os << level_names[static_cast<int>(level)];
  return os;
}


}} // namespace sal::logger
