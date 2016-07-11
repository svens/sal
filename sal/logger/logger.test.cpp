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
  static std::shared_ptr<sal_test::sink_t> sink;

  static void SetUpTestCase ()
  {
    sal::logger::worker_t::make_default(
      sal::logger::set_threshold(sal::logger::level_t::DEBUG),
      sal::logger::set_sink(sink)
    );
  }

  void SetUp () final override
  {
    sink->reset();
  }
};

std::shared_ptr<sal_test::sink_t> logger::sink{
  std::make_shared<sal_test::sink_t>()
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


TEST_P(logger, make_event)
{
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


std::string get_param (const std::string &param, bool &is_called)
{
  is_called = true;
  return param;
}


TEST_P(logger, log)
{
  sal::logger::worker_t worker{
    sal::logger::set_threshold(GetParam()),
    sal::logger::set_sink(sink),
  };

  bool is_called = false;
  sal_log(worker.default_logger(), GetParam())
    << get_param(case_name, is_called);

  ASSERT_TRUE(is_called);
  EXPECT_TRUE(sink->last_message_contains(case_name));
  EXPECT_EQ(GetParam(), sink->last_level);
}


TEST_P(logger, log_disabled)
{
  sal::logger::worker_t worker{
    sal::logger::set_threshold(less_verbose(GetParam())),
    sal::logger::set_sink(sink),
  };

  bool is_called = false;
  sal_log(worker.default_logger(), GetParam())
    << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
  EXPECT_NE(GetParam(), sink->last_level);
}


TEST_P(logger, log_if_true)
{
  sal::logger::worker_t worker{
    sal::logger::set_threshold(GetParam()),
    sal::logger::set_sink(sink),
  };

  bool is_called = false;
  sal_log_if(worker.default_logger(), GetParam(), true)
    << get_param(case_name, is_called);

  ASSERT_TRUE(is_called);
  EXPECT_TRUE(sink->last_message_contains(case_name));
  EXPECT_EQ(GetParam(), sink->last_level);
}


TEST_P(logger, log_if_true_disabled)
{
  sal::logger::worker_t worker{
    sal::logger::set_threshold(less_verbose(GetParam())),
    sal::logger::set_sink(sink),
  };

  bool is_called = false;
  sal_log_if(worker.default_logger(), GetParam(), true)
    << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
  EXPECT_NE(GetParam(), sink->last_level);
}


TEST_P(logger, log_if_false)
{
  sal::logger::worker_t worker{
    sal::logger::set_threshold(GetParam()),
    sal::logger::set_sink(sink),
  };

  bool is_called = false;
  sal_log_if(worker.default_logger(), GetParam(), false)
    << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
  EXPECT_NE(GetParam(), sink->last_level);
}


TEST_P(logger, log_if_false_disabled)
{
  sal::logger::worker_t worker{
    sal::logger::set_threshold(less_verbose(GetParam())),
    sal::logger::set_sink(sink),
  };

  bool is_called = false;
  sal_log_if(worker.default_logger(), GetParam(), false)
    << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
  EXPECT_NE(GetParam(), sink->last_level);
}


INSTANTIATE_TEST_CASE_P(logger, logger, logger_levels);


} // namespace
