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


TEST_P(logger, name)
{
  sal::logger::worker_t worker{
    sal::logger::set_threshold(GetParam())
  };
  auto logger = worker.make_logger(case_name);

  EXPECT_EQ(case_name, logger.name());
}


TEST_P(logger, is_enabled)
{
  sal::logger::worker_t worker{
    sal::logger::set_threshold(GetParam())
  };
  auto logger = worker.make_logger(case_name);

  EXPECT_TRUE(logger.is_enabled(less_verbose(GetParam())));
  EXPECT_TRUE(logger.is_enabled(GetParam()));
  EXPECT_FALSE(logger.is_enabled(more_verbose(GetParam())));
}


struct sink_t
  : public sal::logger::sink_base_t
{
  bool init_called = false, write_called = false;

  void event_init (sal::logger::event_t &) final override
  {
    init_called = true;
  }

  void event_write (sal::logger::event_t &) final override
  {
    write_called = true;
  }
};


TEST_P(logger, make_event)
{
  std::shared_ptr<sink_t> sink = std::make_shared<sink_t>();

  sal::logger::worker_t worker{
    sal::logger::set_threshold(GetParam()),
    sal::logger::set_sink(sink),
  };
  auto logger = worker.make_logger(case_name);

  EXPECT_FALSE(sink->init_called);
  EXPECT_FALSE(sink->write_called);
  {
    auto event = logger.make_event(GetParam());
    EXPECT_TRUE(sink->init_called);
    sink->init_called = false;
    EXPECT_FALSE(sink->write_called);

    EXPECT_EQ(GetParam(), event->level);
    EXPECT_EQ(sal::this_thread::get_id(), event->thread);
    EXPECT_EQ(case_name, *event->logger_name);
  }
  EXPECT_FALSE(sink->init_called);
  EXPECT_TRUE(sink->write_called);
}


INSTANTIATE_TEST_CASE_P(logger, logger, logger_levels);


} // namespace
