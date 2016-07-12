#include <sal/logger/logger.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>


namespace {

using namespace sal_test;


struct logger
  : public fixture
{
  static std::shared_ptr<sal_test::sink_t> sink;

  sal::logger::worker_t worker_;
  sal::logger::logger_t<sal::logger::worker_t> logger_;


  logger ()
    : worker_(sal::logger::set_sink(sink))
    , logger_(worker_.make_logger(case_name))
  {
    sink->reset();
  }


  static void SetUpTestCase ()
  {
    static bool done = false;
    if (!done)
    {
      sal::logger::worker_t::make_default(
        sal::logger::set_sink(sink)
      );
      done = true;
    }
  }
};

std::shared_ptr<sal_test::sink_t> logger::sink{
  std::make_shared<sal_test::sink_t>()
};


TEST_F(logger, name)
{
  EXPECT_EQ(case_name, logger_.name());
}


TEST_F(logger, is_enabled)
{
  EXPECT_TRUE(logger_.is_enabled());

  worker_.set_enabled(logger_, false);
  EXPECT_FALSE(logger_.is_enabled());

  worker_.set_enabled(logger_, true);
  EXPECT_TRUE(logger_.is_enabled());
}


TEST_F(logger, make_event)
{
  EXPECT_FALSE(sink->init_called);
  EXPECT_FALSE(sink->write_called);
  {
    auto event = logger_.make_event();
    EXPECT_TRUE(sink->init_called);
    sink->init_called = false;
    EXPECT_FALSE(sink->write_called);

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


TEST_F(logger, log)
{
  bool is_called = false;
  sal_log(logger_) << get_param(case_name, is_called);

  ASSERT_TRUE(is_called);
  EXPECT_TRUE(sink->last_message_contains(case_name));
}


TEST_F(logger, log_disabled)
{
  worker_.set_enabled(logger_, false);

  bool is_called = false;
  sal_log(logger_) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(logger, log_if_true)
{
  bool is_called = false;
  sal_log_if(logger_, true) << get_param(case_name, is_called);

  ASSERT_TRUE(is_called);
  EXPECT_TRUE(sink->last_message_contains(case_name));
}


TEST_F(logger, log_if_true_disabled)
{
  worker_.set_enabled(logger_, false);

  bool is_called = false;
  sal_log_if(logger_, true) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(logger, log_if_false)
{
  bool is_called = false;
  sal_log_if(logger_, false) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(logger, log_if_false_disabled)
{
  worker_.set_enabled(logger_, false);

  bool is_called = false;
  sal_log_if(logger_, false) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


} // namespace
