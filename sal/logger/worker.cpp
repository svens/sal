#include <sal/logger/worker.hpp>
#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>
#include <sal/assert.hpp>


namespace sal { namespace logger {
__sal_begin


std::unique_ptr<worker_t> worker_t::default_{};


namespace {


#if __apple_build_version__
// TODO drop this section once Xcode8 is released

__thread event_t *this_thread_event_ = nullptr,
  *this_thread_event_ptr_ = nullptr;

inline event_t *this_thread_event_alloc ()
{
  if (!this_thread_event_)
  {
    this_thread_event_ptr_ = this_thread_event_ = new event_t;
  }

  auto result = this_thread_event_ptr_;
  this_thread_event_ptr_ = nullptr;
  return result;
}

inline void this_thread_event_release (event_t *event)
{
  sal_assert(this_thread_event_ == event);
  this_thread_event_ptr_ = event;
}

#else // if __apple_build_version__

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

#endif // else __apple_build_version__


} // namespace


event_t *worker_t::alloc_and_init (const channel_type &channel)
{
  auto event = sal_check_ptr(this_thread_event_alloc());

  try
  {
    event->message.reset();
    event->sink = channel.impl_.sink.get();
    event->sink->sink_event_init(*event, channel.name());
    return event;
  }
  catch (...)
  {
    this_thread_event_release(event);
  }

  return nullptr;
}


void worker_t::write_and_release (event_t *event)
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


__sal_end
}} // namespace sal::logger
