#include <sal/logger/worker.hpp>
#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>


namespace sal { namespace logger {
__sal_begin


event_t *worker_t::alloc_and_init (level_t level, const logger_type &logger)
  noexcept
{
  if (auto event = new (std::nothrow) event_t)
  {
    try
    {
      event->level = level;
      event->time = now();
      event->thread = this_thread::get_id();
      event->message.reset();
      event->logger_name = &logger.name();
      event->sink = logger.impl_.sink.get();
      event->sink->init(*event);
      return event;
    }
    catch (...)
    {
      delete event;
    }
  }

  return nullptr;
}


void worker_t::write_and_release (event_t *event) noexcept
{
  try
  {
    event->sink->write(*event);
  }
  catch (...)
  {}

  delete event;
}


__sal_end
}} // namespace sal::logger
