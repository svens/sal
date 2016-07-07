#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>
#include <sal/logger/logger.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>


namespace {


using namespace sal_test;
using logger = with_value<sal::logger::level_t>;


#if 0
class test_sink_t
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
#endif


TEST_P(logger, print)
{
  using namespace sal::logger;

  const auto level = GetParam();

  worker_t loggers{
    set_threshold(level),
  };

  auto first = loggers.make_logger("first");
  EXPECT_EQ("first", first.name());
  EXPECT_TRUE(first.is_enabled(less_verbose(level)));
  EXPECT_FALSE(first.is_enabled(more_verbose(level)));

  auto another_first = first;
  EXPECT_EQ("first", another_first.name());
  EXPECT_TRUE(another_first.is_enabled(less_verbose(level)));
  EXPECT_FALSE(another_first.is_enabled(more_verbose(level)));

  auto second = loggers.make_logger("second",
    set_threshold(more_verbose(level))
  );
  EXPECT_EQ("second", second.name());
  EXPECT_TRUE(second.is_enabled(level));
  EXPECT_FALSE(second.is_enabled(more_verbose(more_verbose(level))));

  auto another_second = second;
  EXPECT_EQ("second", another_second.name());
  EXPECT_TRUE(another_second.is_enabled(level));
  EXPECT_FALSE(another_second.is_enabled(more_verbose(more_verbose(level))));
}


INSTANTIATE_TEST_CASE_P(logger, logger, logger_levels);


} // namespace
