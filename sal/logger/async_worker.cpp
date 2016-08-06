#include <sal/logger/async_worker.hpp>
#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>
#include <sal/queue.hpp>
#include <sal/spinlock.hpp>
#include <deque>
#include <mutex>
#include <thread>


namespace sal { namespace logger {
__sal_begin


struct async_worker_t::impl_t
{
  struct event_ctl_t
    : public event_t
  {
    impl_t *owner;
    spsc_t free_hook{};
    mpsc_t write_hook{};

    event_ctl_t (impl_t *owner) noexcept
      : owner(owner)
    {}
  };

  spinlock_t mutex{};
  std::deque<event_ctl_t> pool{};
  queue_t<event_ctl_t, spsc_t, &event_ctl_t::free_hook> free_list{};
  alignas(64) queue_t<event_ctl_t, mpsc_t, &event_ctl_t::write_hook> write_list{};
  std::thread writer{};


  impl_t ()
  {
    while (pool.size() < 2 * std::thread::hardware_concurrency())
    {
      pool.emplace_back(this);
      free_list.push(&pool.back());
    }
  }


  event_t *make_event () noexcept
  {
    std::lock_guard<spinlock_t> lock(mutex);

    if (auto event_ctl = free_list.try_pop())
    {
      return static_cast<event_t *>(event_ctl);
    }

    pool.emplace_back(this);
    return static_cast<event_t *>(&pool.back());
  }


  void release (event_t *event) noexcept
  {
    free_list.push(static_cast<event_ctl_t *>(event));
  }


  static void async_write (event_t *event) noexcept
  {
    auto event_ctl = static_cast<event_ctl_t *>(event);
    event_ctl->owner->write_list.push(event_ctl);
  }


  void event_writer () noexcept;


  static event_ctl_t *stop_event () noexcept
  {
    static event_ctl_t event{nullptr};
    return &event;
  }


  static void stop_event_writer (impl_t *impl)
  {
    // wrap impl again into unique_ptr, this time with real delete
    std::unique_ptr<impl_t> guard(impl);

    if (guard->writer.joinable())
    {
      guard->write_list.push(stop_event());
      guard->writer.join();
    }
  }

#if __sal_os_windows

  // MSVC: warning C4316: object allocated on the heap may not be aligned 64

  void *operator new (size_t size)
  {
    return _mm_malloc(size, 64);
  }

  void operator delete (void *p)
  {
    _mm_free(p);
  }

#endif
};


void async_worker_t::impl_t::event_writer () noexcept
{
  // wait and write events until stop_event received
  for (auto i = 0U;  /**/;  /**/)
  {
    auto event_ctl = write_list.try_pop();
    if (!event_ctl)
    {
      // no event
      adaptive_spin<100>(i++);
      continue;
    }

    if (event_ctl == stop_event())
    {
      // stop_event
      break;
    }

    try
    {
      // write event
      event_ctl->sink->sink_event_write(*event_ctl);
      i = 0;
    }
    catch (...)
    {
    }

    release(event_ctl);
  }

  // write remaining events (if any)
  while (auto event_ctl = write_list.try_pop())
  {
    event_ctl->sink->sink_event_write(*event_ctl);
  }
}


async_worker_t::impl_ptr async_worker_t::start ()
{
  auto impl = impl_ptr{new impl_t, &impl_t::stop_event_writer};
  impl->writer = std::thread(&impl_t::event_writer, impl.get());
  return impl;
}


event_ptr async_worker_t::make_event (const channel_type &channel) noexcept
{
  event_ptr event(impl_->make_event(), &impl_t::async_write);

  if (event)
  {
    try
    {
      event->message.reset();
      event->sink = channel.impl_.sink.get();
      event->sink->sink_event_init(*event, channel.name());
    }
    catch (...)
    {
      impl_->release(event.release());
    }
  }

  return event;
}


__sal_end
}} // namespace sal::logger
