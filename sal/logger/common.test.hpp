#pragma once

#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>
#include <sal/common.test.hpp>


namespace sal_test {


struct sink_t final
  : public sal::logger::sink_t
{
  bool init_called, write_called;
  bool throw_init, throw_write;
  std::string last_message;

  sink_t ()
  {
    reset();
  }

  void reset ()
  {
    init_called = write_called = throw_init = throw_write = false;
    last_message = "";
  }

  void sink_event_init (sal::logger::event_t &event,
    const std::string &channel_name) override
  {
    sal::logger::sink_t::sink_event_init(event, channel_name);

    init_called = true;
    if (throw_init)
    {
      throw_init = false;
      throw false;
    }
  }

  void sink_event_write (sal::logger::event_t &event) override
  {
    write_called = true;
    if (throw_write)
    {
      throw_write = false;
      throw false;
    }
    last_message = sal::to_string(event.message);
  }

  bool last_message_contains (const std::string &value)
  {
    return last_message.find(value) != last_message.npos;
  }
};


} // namespace sal_test
