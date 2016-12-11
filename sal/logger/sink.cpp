#include <sal/logger/sink.hpp>
#include <sal/builtins.hpp>
#include <sal/spinlock.hpp>
#include <sal/thread.hpp>
#include <sal/time.hpp>
#include <iostream>
#include <mutex>


__sal_begin


namespace logger { namespace {


inline void split_time_of_day (time_t time,
  unsigned &h,
  unsigned &m,
  unsigned &s,
  unsigned &ms)
{
  using namespace std;
  using namespace std::chrono;
  using days = duration<int, ratio_multiply<hours::period, ratio<24>>::type>;

  // skip days, leaving timestamp since 00:00:00.000
  auto t = time.time_since_epoch();
  t -= duration_cast<days>(t);

  auto hh = duration_cast<hours>(t);
  h = hh.count();
  t -= hh;

  auto mm = duration_cast<minutes>(t);
  m = mm.count();
  t -= mm;

  auto ss = duration_cast<seconds>(t);
  s = static_cast<unsigned>(ss.count());
  t -= ss;

  ms = static_cast<unsigned>(
    t.count() * 1000 / clock_t::duration::period::den
  );
}


} // namespace


time_t sink_t::local_now () noexcept
{
  auto time = now();

  // querying local_offset is relatively expensive, use double-checked locking
  // and update static bias once per interval

  using namespace std::chrono_literals;

  static auto bias = 0s;
  static time_t next_update = time;
  constexpr auto interval = 1s;

  if (sal_unlikely(time >= next_update))
  {
    static spinlock_t mutex;
    std::lock_guard<spinlock_t> lock(mutex);

    if (time >= next_update)
    {
      bias = local_offset(time);
      next_update += interval;
    }
  }

  return time + bias;
}


void sink_t::init (event_t &event, const std::string &channel_name) noexcept
{
  //
  // hh:mm:ss.msec\t
  //

  unsigned h, m, s, ms;
  split_time_of_day(event.time, h, m, s, ms);

  // hour:
  if (h < 10) event.message << '0';     // LCOV_EXCL_LINE
  event.message << h << ':';

  // minute:
  if (m < 10) event.message << '0';     // LCOV_EXCL_LINE
  event.message << m << ':';

  // seconds.
  if (s < 10) event.message << '0';     // LCOV_EXCL_LINE
  event.message << s << ',';

  // milliseoconds
  if (ms < 100) event.message << '0';   // LCOV_EXCL_LINE
  if (ms < 10) event.message << '0';    // LCOV_EXCL_LINE
  event.message << ms << '\t';

  //
  // thread\t
  //

  event.message << this_thread::get_id() << '\t';

  //
  // '[module] '

  if (!channel_name.empty())
  {
    event.message << '[' << channel_name << "] ";
  }
}


namespace {


struct ostream_sink_t final
  : public sink_t
{
  std::ostream &ostream;

  ostream_sink_t (std::ostream &ostream)
    : ostream(ostream)
  {}

  void sink_event_write (event_t &event) final override
  {
    ostream
      << (event.message.good() ? event.message.c_str() : "<...>")
      << '\n';
  }
};


} // namespace


sink_ptr ostream_sink (std::ostream &os)
{
  if (&os == &std::cout)
  {
    static auto sink = std::make_shared<ostream_sink_t>(std::cout);
    return sink;
  }
  else if (&os == &std::cerr)
  {
    static auto sink = std::make_shared<ostream_sink_t>(std::cerr);
    return sink;
  }
  return std::make_shared<ostream_sink_t>(os);
}


} // namespace logger


__sal_end
