#include <sal/logger/async_worker.hpp>
#include <sal/logger/logger.hpp>
#include <sal/logger/sink.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>


namespace {


using namespace sal::logger;


template <typename Worker>
struct worker
  : public sal_test::with_type<Worker>
{
};


TYPED_TEST_CASE_P(worker);


TYPED_TEST_P(worker, default_logger_name)
{
  TypeParam worker;
  auto logger = worker.default_logger();
  EXPECT_EQ("", logger.name());
}


TYPED_TEST_P(worker, default_logger_is_enabled)
{
  TypeParam worker;

  auto logger = worker.default_logger();
  EXPECT_TRUE(logger.is_enabled());

  worker.set_enabled(logger, false);
  EXPECT_FALSE(logger.is_enabled());

  worker.set_enabled(logger, true);
  EXPECT_TRUE(logger.is_enabled());
}


TYPED_TEST_P(worker, default_logger_sink)
{
  auto sink = std::make_shared<sal_test::sink_t>();
  TypeParam worker{set_sink(sink)};
  auto logger = worker.default_logger();

  EXPECT_FALSE(sink->init_called);
  EXPECT_FALSE(sink->write_called);
  logger.make_event()->message << this->case_name;
  EXPECT_TRUE(sink->init_called);
  EXPECT_TRUE(sink->write_called);
  EXPECT_TRUE(sink->last_message_contains(this->case_name));
}


TYPED_TEST_P(worker, get_logger_default)
{
  TypeParam worker;
  auto logger = worker.get_logger(this->case_name);
  EXPECT_EQ("", logger.name());
}


TYPED_TEST_P(worker, make_logger)
{
  auto sink = std::make_shared<sal_test::sink_t>();
  TypeParam worker{set_sink(sink)};

  auto returned_logger = worker.make_logger(this->case_name);
  EXPECT_EQ(this->case_name, returned_logger.name());

  auto got_logger = worker.get_logger(this->case_name);
  EXPECT_EQ(this->case_name, got_logger.name());
}


TYPED_TEST_P(worker, sink_throwing_event_init)
{
  auto sink = std::make_shared<sal_test::sink_t>();
  TypeParam worker{set_sink(sink)};
  auto logger = worker.default_logger();

  sink->throw_init = true;
  EXPECT_EQ(nullptr, logger.make_event());
  EXPECT_TRUE(sink->init_called);
  EXPECT_FALSE(sink->write_called);
}


TYPED_TEST_P(worker, sink_throwing_event_write)
{
  auto sink = std::make_shared<sal_test::sink_t>();
  TypeParam worker{set_sink(sink)};

  {
    auto logger = worker.default_logger();
    sink->throw_write = true;
    auto event = logger.make_event();
    ASSERT_NE(nullptr, event);
    event->message << this->case_name;
  }
  EXPECT_TRUE(sink->init_called);
  EXPECT_TRUE(sink->write_called);
  EXPECT_EQ("", sink->last_message);
}


REGISTER_TYPED_TEST_CASE_P(worker,
  default_logger_name,
  default_logger_is_enabled,
  default_logger_sink,
  get_logger_default,
  make_logger,
  sink_throwing_event_init,
  sink_throwing_event_write
);


using worker_types = testing::Types<
  sal::logger::worker_t,
  sal::logger::async_worker_t
>;


INSTANTIATE_TYPED_TEST_CASE_P(logger, worker, worker_types);


} // namespace
