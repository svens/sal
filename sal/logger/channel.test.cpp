#include <sal/logger/channel.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>


namespace {

using namespace sal_test;


struct channel
  : public fixture
{
  std::shared_ptr<sal_test::sink_t> sink_;
  sal::logger::worker_t worker_;
  sal::logger::channel_t<sal::logger::worker_t> channel_;

  channel ()
    : sink_(std::make_shared<sal_test::sink_t>())
    , worker_(sal::logger::set_sink(sink_))
    , channel_(worker_.make_channel(case_name))
  {}
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
  EXPECT_FALSE(sink_->init_called);
  EXPECT_FALSE(sink_->write_called);
  {
    auto event = channel_.make_event();
    EXPECT_TRUE(sink_->init_called);
    sink_->init_called = false;
    EXPECT_FALSE(sink_->write_called);

    EXPECT_EQ(sal::this_thread::get_id(), event->thread);
    EXPECT_EQ(case_name, *event->channel_name);
  }
  EXPECT_FALSE(sink_->init_called);
  EXPECT_TRUE(sink_->write_called);
}


} // namespace
