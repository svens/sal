#include <sal/logger/logger.hpp>
#include <sal/logger/common.test.hpp>


namespace {

using namespace sal_test;


struct logger
  : public fixture
{
  static std::shared_ptr<sal_test::sink_t> sink;

  sal::logger::worker_t worker_;
  sal::logger::channel_t<sal::logger::worker_t> channel_;


  logger ()
    : worker_(sal::logger::set_sink(sink))
    , channel_(worker_.make_channel(case_name))
  {
    sink->reset();
    set_enabled_default_channel(true);
  }


  static void SetUpTestCase ()
  {
    static bool done = false;
    if (!done)
    {
      sal::logger::make_default_worker(
        sal::logger::set_sink(sink)
      );
      done = true;
    }
  }


  void set_enabled_default_channel (bool enable)
  {
    sal::logger::default_worker().set_enabled(
      sal::logger::default_channel(),
      enable
    );
  }
};

std::shared_ptr<sal_test::sink_t> logger::sink{
  std::make_shared<sal_test::sink_t>()
};


std::string get_param (const std::string &param, bool &is_called)
{
  is_called = true;
  return param;
}


TEST_F(logger, log)
{
  bool is_called = false;
  sal_log(channel_) << get_param(case_name, is_called);

  ASSERT_TRUE(is_called);
  EXPECT_TRUE(sink->last_message_contains(case_name));
}


TEST_F(logger, log_disabled)
{
  worker_.set_enabled(channel_, false);

  bool is_called = false;
  sal_log(channel_) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(logger, log_if_true)
{
  bool is_called = false;
  sal_log_if(channel_, true) << get_param(case_name, is_called);

  ASSERT_TRUE(is_called);
  EXPECT_TRUE(sink->last_message_contains(case_name));
}


TEST_F(logger, log_if_true_disabled)
{
  worker_.set_enabled(channel_, false);

  bool is_called = false;
  sal_log_if(channel_, true) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(logger, log_if_false)
{
  bool is_called = false;
  sal_log_if(channel_, false) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(logger, log_if_false_disabled)
{
  worker_.set_enabled(channel_, false);

  bool is_called = false;
  sal_log_if(channel_, false) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(logger, print)
{
  bool is_called = false;
  sal_print << get_param(case_name, is_called);

  ASSERT_TRUE(is_called);
  EXPECT_TRUE(sink->last_message_contains(case_name));
}


TEST_F(logger, print_disabled)
{
  set_enabled_default_channel(false);

  bool is_called = false;
  sal_print << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(logger, print_if_true)
{
  bool is_called = false;
  sal_print_if(true) << get_param(case_name, is_called);

  ASSERT_TRUE(is_called);
  EXPECT_TRUE(sink->last_message_contains(case_name));
}


TEST_F(logger, print_if_true_disabled)
{
  set_enabled_default_channel(false);

  bool is_called = false;
  sal_print_if(true) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(logger, print_if_false)
{
  bool is_called = false;
  sal_print_if(false) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(logger, print_if_false_disabled)
{
  set_enabled_default_channel(false);

  bool is_called = false;
  sal_print_if(false) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


} // namespace
