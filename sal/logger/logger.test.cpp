#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>
#include <sal/logger/logger.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>


namespace {


using namespace sal_test;


struct logger
  : public with_value<sal::logger::level_t>
{
};


class sink_t
  : public sal::logger::sink_base_t
{
public:

  void event_init (sal::logger::event_t &event) final override
  {
    event.message
      << event.level
      << '\t' << event.thread
      << '\t' << '[' << *event.logger_name << ']'
      << ' ';
  }

  void event_write (sal::logger::event_t &event) final override
  {
    event.message << '.';
    std::cout << event.message.get() << std::endl;
  }
};


TEST_P(logger, name)
{
  using namespace sal::logger;
}


TEST_P(logger, is_enabled)
{
  using namespace sal::logger;
}


TEST_P(logger, make_event)
{
  using namespace sal::logger;
}


INSTANTIATE_TEST_CASE_P(logger, logger, logger_levels);


} // namespace
