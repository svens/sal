#include <sal/logger/channel.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>


namespace {

using namespace sal_test;


struct channel
  : public fixture
{
  static std::shared_ptr<sal_test::sink_t> sink;

  sal::logger::worker_t worker_;
  sal::logger::channel_t<sal::logger::worker_t> channel_;


  channel ()
    : worker_(sal::logger::set_sink(sink))
    , channel_(worker_.make_channel(case_name))
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

std::shared_ptr<sal_test::sink_t> channel::sink{
  std::make_shared<sal_test::sink_t>()
};


TEST_F(channel, name)
{
  EXPECT_EQ(case_name, channel_.name());
}


TEST_F(channel, is_enabled)
{
  EXPECT_TRUE(channel_.is_enabled());

  worker_.set_enabled(channel_, false);
  EXPECT_FALSE(channel_.is_enabled());

  worker_.set_enabled(channel_, true);
  EXPECT_TRUE(channel_.is_enabled());
}


TEST_F(channel, make_event)
{
  EXPECT_FALSE(sink->init_called);
  EXPECT_FALSE(sink->write_called);
  {
    auto event = channel_.make_event();
    EXPECT_TRUE(sink->init_called);
    sink->init_called = false;
    EXPECT_FALSE(sink->write_called);

    EXPECT_EQ(sal::this_thread::get_id(), event->thread);
    EXPECT_EQ(case_name, *event->channel_name);
  }
  EXPECT_FALSE(sink->init_called);
  EXPECT_TRUE(sink->write_called);
}


std::string get_param (const std::string &param, bool &is_called)
{
  is_called = true;
  return param;
}


TEST_F(channel, log)
{
  bool is_called = false;
  sal_log(channel_) << get_param(case_name, is_called);

  ASSERT_TRUE(is_called);
  EXPECT_TRUE(sink->last_message_contains(case_name));
}


TEST_F(channel, log_disabled)
{
  worker_.set_enabled(channel_, false);

  bool is_called = false;
  sal_log(channel_) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(channel, log_if_true)
{
  bool is_called = false;
  sal_log_if(channel_, true) << get_param(case_name, is_called);

  ASSERT_TRUE(is_called);
  EXPECT_TRUE(sink->last_message_contains(case_name));
}


TEST_F(channel, log_if_true_disabled)
{
  worker_.set_enabled(channel_, false);

  bool is_called = false;
  sal_log_if(channel_, true) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(channel, log_if_false)
{
  bool is_called = false;
  sal_log_if(channel_, false) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


TEST_F(channel, log_if_false_disabled)
{
  worker_.set_enabled(channel_, false);

  bool is_called = false;
  sal_log_if(channel_, false) << get_param(case_name, is_called);

  ASSERT_FALSE(is_called);
  EXPECT_FALSE(sink->last_message_contains(case_name));
}


} // namespace
