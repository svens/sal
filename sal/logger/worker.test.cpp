#include <sal/logger/async_worker.hpp>
#include <sal/logger/logger.hpp>
#include <sal/logger/sink.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>


namespace {


template <typename Worker>
struct worker
  : public sal_test::with_type<Worker>
{
};


struct sink_t final
  : public sal::logger::sink_base_t
{
  bool init_called = false, write_called = false;
  bool throw_init = false, throw_write = false;
  std::string last_message;

  void event_init (sal::logger::event_t &event) override
  {
    sink_base_t::event_init(event);

    init_called = true;
    if (throw_init)
    {
      throw_init = false;
      throw false;
    }
  }

  void event_write (sal::logger::event_t &event) override
  {
    write_called = true;
    if (throw_write)
    {
      throw_write = false;
      throw false;
    }
    last_message = sal::to_string(event.message);
  }
};


TYPED_TEST_CASE_P(worker);


TYPED_TEST_P(worker, default_logger_name)
{
  using namespace sal::logger;

  TypeParam worker;
  auto logger = worker.default_logger();
  EXPECT_EQ("", logger.name());
}


TYPED_TEST_P(worker, default_logger_level)
{
  using namespace sal::logger;

  TypeParam worker{
    set_threshold(level_t::WARN),
  };

  auto logger = worker.default_logger();
  EXPECT_TRUE(logger.is_enabled(level_t::WARN));
  EXPECT_FALSE(logger.is_enabled(level_t::INFO));
}


TYPED_TEST_P(worker, default_logger_sink)
{
  using namespace sal::logger;

  auto sink = std::make_shared<sink_t>();

  TypeParam worker{
    set_sink(sink),
  };

  auto logger = worker.default_logger();

  EXPECT_FALSE(sink->init_called);
  EXPECT_FALSE(sink->write_called);
  logger.make_event(level_t::INFO)->message << this->case_name;
  EXPECT_TRUE(sink->init_called);
  EXPECT_TRUE(sink->write_called);
  EXPECT_EQ(this->case_name, sink->last_message);
}


TYPED_TEST_P(worker, get_logger_default)
{
  using namespace sal::logger;

  TypeParam worker;
  auto logger = worker.get_logger(this->case_name);
  EXPECT_EQ("", logger.name());
}


TYPED_TEST_P(worker, make_logger)
{
  using namespace sal::logger;

  auto sink = std::make_shared<sink_t>();
  TypeParam worker{set_sink(sink)};

  auto returned_logger = worker.make_logger(this->case_name);
  EXPECT_EQ(this->case_name, returned_logger.name());

  auto got_logger = worker.get_logger(this->case_name);
  EXPECT_EQ(this->case_name, got_logger.name());
}


TYPED_TEST_P(worker, sink_throwing_event_init)
{
  using namespace sal::logger;

  auto sink = std::make_shared<sink_t>();
  TypeParam worker{set_sink(sink)};

  auto logger = worker.default_logger();
  sink->throw_init = true;
  EXPECT_EQ(nullptr, logger.make_event(level_t::INFO));
  EXPECT_TRUE(sink->init_called);
  EXPECT_FALSE(sink->write_called);
}


TYPED_TEST_P(worker, sink_throwing_event_write)
{
  using namespace sal::logger;

  auto sink = std::make_shared<sink_t>();
  TypeParam worker{set_sink(sink)};

  {
    auto logger = worker.default_logger();
    sink->throw_write = true;
    auto event = logger.make_event(level_t::INFO);
    EXPECT_NE(nullptr, event);
    event->message << this->case_name;
  }
  EXPECT_TRUE(sink->init_called);
  EXPECT_TRUE(sink->write_called);
  EXPECT_EQ("", sink->last_message);
}


REGISTER_TYPED_TEST_CASE_P(worker,
  default_logger_name,
  default_logger_level,
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
