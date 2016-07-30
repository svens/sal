#include <sal/logger/async_worker.hpp>
#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>


namespace sal { namespace logger {
__sal_begin


event_t *async_worker_t::alloc_and_init (const channel_type &channel) noexcept
{
  if (auto event = new (std::nothrow) event_t)
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
      delete event;
    }
  }

  return nullptr;
}


void async_worker_t::write_and_release (event_t *event) noexcept
{
  try
  {
    event->sink->sink_event_write(*event);
  }
  catch (...)
  {}

  delete event;
}


__sal_end
}} // namespace sal::logger
