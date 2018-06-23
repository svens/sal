#include <sal/logger/async_worker.hpp>
#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>
#include <sal/intrusive_mpsc_queue.hpp>
#include <sal/spinlock.hpp>
#include <deque>
#include <mutex>
#include <thread>


__sal_begin


namespace logger {


struct async_worker_t::impl_t
{
  struct event_ctl_t
    : public event_t
  {
    union
    {
      intrusive_mpsc_queue_hook_t<event_ctl_t> free_hook;
      intrusive_mpsc_queue_hook_t<event_ctl_t> write_hook;
    };

    using free_list_t = intrusive_mpsc_queue_t<&event_ctl_t::free_hook>;
    free_list_t * const free_list{};

    using write_list_t = intrusive_mpsc_queue_t<&event_ctl_t::write_hook>;
    write_list_t * const write_list{};

    event_ctl_t (free_list_t *free_list, write_list_t *write_list) noexcept
      : free_list(free_list)
      , write_list(write_list)
    {}
  };

  struct event_pool_t
  {
    spinlock_t mutex{};
    std::deque<event_ctl_t> pool{};
    event_ctl_t::free_list_t free_list{};
  };

  std::thread writer{};
  event_ctl_t::write_list_t write_list{};
  std::deque<event_pool_t> free_list_segments{2};


  impl_t ()
  {
    for (auto &segment: free_list_segments)
    {
      segment.pool.emplace_back(&segment.free_list, &write_list);
      segment.free_list.push(&segment.pool.back());
    }
  }


  event_t *make_event () noexcept
  {
    static std::atomic<unsigned> quasi_rnd{};
    auto &segment = free_list_segments[++quasi_rnd % free_list_segments.size()];
    std::lock_guard<spinlock_t> lock(segment.mutex);

    if (auto event_ctl = segment.free_list.try_pop())
    {
      return static_cast<event_t *>(event_ctl);
    }

    segment.pool.emplace_back(&segment.free_list, &write_list);
    return static_cast<event_t *>(&segment.pool.back());
  }


  void release (event_t *event) noexcept
  {
    auto event_ctl = static_cast<event_ctl_t *>(event);
    event_ctl->free_list->push(event_ctl);
  }


  static void async_write (event_t *event) noexcept
  {
    auto event_ctl = static_cast<event_ctl_t *>(event);
    event_ctl->write_list->push(event_ctl);
  }


  void event_writer () noexcept;


  static event_ctl_t *stop_event () noexcept
  {
    static event_ctl_t event{nullptr, nullptr};
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
}


async_worker_t::impl_ptr async_worker_t::start ()
{
  auto impl = impl_ptr{new impl_t, &impl_t::stop_event_writer};
  impl->writer = std::thread(&impl_t::event_writer, impl.get());
  return impl;
}


event_ptr async_worker_t::make_event (const channel_type &channel) noexcept
{
  event_ptr event_p(impl_->make_event(), &impl_t::async_write);
  if (event_p)
  {
    try
    {
      auto &event = *event_p;
      event.message.reset();
      event.sink = channel.impl_.sink.get();
      event.sink->sink_event_init(event, channel.name());
    }
    catch (...)
    {
      impl_->release(event_p.release());
    }
  }
  return event_p;
}


} // namespace logger


__sal_end
