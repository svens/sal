#include <sal/logger/worker.hpp>
#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>
#include <sal/assert.hpp>


__sal_begin


namespace logger {


std::unique_ptr<worker_t> worker_t::default_{};


namespace {


thread_local event_t this_thread_event_{},
  *this_thread_event_ptr_ = &this_thread_event_;

inline event_t *this_thread_event_alloc ()
{
  auto result = this_thread_event_ptr_;
  this_thread_event_ptr_ = nullptr;
  return result;
}


inline void this_thread_event_release (event_t *event)
{
  // note: this function is called from unique_ptr custom deleter that's not
  // supposed to throw exception but we have no choice, let's rather fail
  // quickly than allow to mess with contract
  sal_assert(&this_thread_event_ == event);
  this_thread_event_ptr_ = event;
}


void write_and_release (event_t *event)
{
  // here it's ok to release event and still keep using it
  this_thread_event_release(event);

  try
  {
    event->sink->sink_event_write(*event);
  }
  catch (...)
  {
  }
}


} // namespace


event_ptr worker_t::make_event (const channel_type &channel)
{
  event_ptr event(sal_check_ptr(this_thread_event_alloc()), &write_and_release);

  try
  {
    event->message.reset();
    event->sink = channel.impl_.sink.get();
    event->sink->sink_event_init(*event, channel.name());
  }
  catch (...)
  {
    this_thread_event_release(event.release());
  }

  return event;
}


} // namespace logger


__sal_end
