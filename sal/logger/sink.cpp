#include <sal/logger/sink.hpp>
#include <sal/logger/event.hpp>
#include <sal/assert.hpp>
#include <sal/thread.hpp>
#include <sal/time.hpp>
#include <cstdio>


namespace sal { namespace logger {
__sal_begin


namespace {


void split_now (unsigned &h, unsigned &m, unsigned &s, unsigned &ms)
{
  using namespace std;
  using namespace std::chrono;
  using days = duration<int, ratio_multiply<hours::period, ratio<24>>::type>;

  // skip days, leaving timestamp since 00:00:00.000
  auto t = now().time_since_epoch();
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
  split_now(h, m, s, ms);

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


void sink_t::sink_event_init (event_t &event, const std::string &channel_name)
{
  // hh:mm:ss.msec\t
  insert_time(event) << '\t';

  // thread\t
  event.message << this_thread::get_id() << '\t';

  // '[module] '
  if (!channel_name.empty())
  {
    event.message << '[' << channel_name << "] ";
  }
}


namespace {


struct FILE_sink_t final
  : public sink_t
{
  FILE *file_{};


  FILE_sink_t () = default;
  FILE_sink_t (const FILE_sink_t &) = default;
  FILE_sink_t &operator= (const FILE_sink_t &) = default;


  FILE_sink_t (FILE *file)
    : file_(sal_check_ptr(file))
  {}


  void sink_event_write (event_t &event) final override
  {
    if (event.message.good())
    {
      fprintf(file_, "%s\n", event.message.get());
    }
    else
    {
      // message is truncated, append marker
      // (message itself is always NUL-terminated)
      fprintf(file_, "%s<...>\n", event.message.get());
    }
  }
};


} // namespace


sink_ptr stdout_sink ()
{
  static auto sink_ = std::make_shared<FILE_sink_t>(stdout);
  return sink_;
}


sink_ptr stderr_sink ()
{
  static auto sink_ = std::make_shared<FILE_sink_t>(stderr);
  return sink_;
}


__sal_end
}} // namespace sal::logger
