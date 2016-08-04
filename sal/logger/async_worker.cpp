#include <sal/logger/async_worker.hpp>
#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>


namespace sal { namespace logger {
__sal_begin


namespace {


void write_and_release (event_t *event) noexcept
{
  try
  {
    event->sink->sink_event_write(*event);
  }
  catch (...)
  {}

  delete event;
}


} // namespace


event_ptr async_worker_t::make_event (const channel_type &channel) noexcept
{
  event_ptr event(new (std::nothrow) event_t, &write_and_release);

  if (event)
  {
    try
    {
      event->message.reset();
      event->sink = channel.impl_.sink.get();
      event->sink->sink_event_init(*event, channel.name());
      return event;
    }
    catch (...)
    {
      delete event.release();
    }
  }

  return event;
}


__sal_end
}} // namespace sal::logger
