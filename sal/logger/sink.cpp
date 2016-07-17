#include <sal/logger/sink.hpp>
#include <sal/logger/event.hpp>
#include <sal/assert.hpp>
#include <chrono>
#include <cstdio>


namespace sal { namespace logger {
__sal_begin


namespace {


void split_time (event_t &event,
  unsigned &h, unsigned &m, unsigned &s, unsigned &ms)
{
  using namespace std;
  using namespace std::chrono;
  using days = duration<int, ratio_multiply<hours::period, ratio<24>>::type>;

  // skip days, leaving timestamp since 00:00:00.000
  clock_t::duration t = event.time.time_since_epoch();
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


auto &insert_time (event_t &event) noexcept
{
  unsigned h, m, s, ms;
  split_time(event, h, m, s, ms);

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
  event.message << ms;

  return event.message;
}


} // namespace


void sink_t::event_init (event_t &event)
{
  // hh:mm:ss.msec\t
  insert_time(event) << '\t';

  // thread\t
  event.message << event.thread << '\t';

  // '[module] '
  if (event.channel_name && !event.channel_name->empty())
  {
    event.message << '[' << *event.channel_name << "] ";
  }
}


void sink_t::event_write (event_t &event)
{
  if (event.message.good())
  {
    printf("%s\n", event.message.get());
  }
  else
  {
    // message is truncated, append marker
    // (message itself is always NUL-terminated)
    printf("%s<...>\n", event.message.get());
  }
}


__sal_end
}} // namespace sal::logger
