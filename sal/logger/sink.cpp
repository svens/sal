#include <sal/logger/sink.hpp>
#include <sal/logger/level.hpp>
#include <sal/assert.hpp>
#include <chrono>
#include <cstdio>


namespace sal { namespace logger {
__sal_begin


namespace {


inline constexpr char level_char (level_t level)
{
#if _MSC_VER
  // TODO: drop once MSVC supports C++14 constexpr

  return level == level_t::INFO ? 'i'
    : level == level_t::ERROR ? 'e'
    : level == level_t::WARN ? 'w'
    : level == level_t::DEBUG ? 'd'
    : '?'
  ;

#else

  switch (level)
  {
    case level_t::ERROR: return 'e';
    case level_t::WARN: return 'w';
    case level_t::INFO: return 'i';
    case level_t::DEBUG: return 'd';
  }
  return '?';

#endif
}


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
  event.message << s << '.';

  // milliseoconds
  if (ms < 100) event.message << '0';   // LCOV_EXCL_LINE
  if (ms < 10) event.message << '0';    // LCOV_EXCL_LINE
  event.message << ms;

  return event.message;
}


} // namespace


void sink_t::event_init (event_t &event)
{
  // level,
  event.message << level_char(event.level) << ',';

  // hh:mm:ss.msec,
  insert_time(event) << ',';

  // thread\t
  event.message << event.thread << '\t';

  // '[module] '
  if (event.logger_name && !event.logger_name->empty())
  {
    event.message << '[' << *event.logger_name << "] ";
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
