#include <sal/logger/channel.hpp>
#include <sal/logger/async_worker.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>


namespace {


template <typename Worker>
struct channel
  : public sal_test::with_type<Worker>
{
  std::shared_ptr<sal_test::sink_t> sink_;
  Worker worker_;
  sal::logger::channel_t<Worker> channel_;

  channel ()
    : sink_(std::make_shared<sal_test::sink_t>())
    , worker_(sal::logger::set_sink(sink_))
    , channel_(worker_.make_channel(this->case_name))
  {}
};

TYPED_TEST_CASE_P(channel);


TYPED_TEST_P(channel, name)
{
  EXPECT_EQ(this->case_name, this->channel_.name());
}


TYPED_TEST_P(channel, enabled)
{
  EXPECT_TRUE(this->channel_.is_enabled());

  this->channel_.set_enabled(false);
  EXPECT_FALSE(this->channel_.is_enabled());

  this->channel_.set_enabled(true);
  EXPECT_TRUE(this->channel_.is_enabled());
}


TYPED_TEST_P(channel, make_event)
{
  EXPECT_FALSE(this->sink_->init_called);
  EXPECT_FALSE(this->sink_->write_called);
  {
    auto event = this->channel_.make_event();
    EXPECT_TRUE(this->sink_->init_called);
    this->sink_->init_called = false;
    EXPECT_FALSE(this->sink_->write_called);

    EXPECT_EQ(sal::this_thread::get_id(), event->thread);
    EXPECT_EQ(this->case_name, *event->channel_name);
  }
  EXPECT_FALSE(this->sink_->init_called);
  EXPECT_TRUE(this->sink_->write_called);
}


REGISTER_TYPED_TEST_CASE_P(channel,
  name,
  enabled,
  make_event
);


using worker_types = testing::Types<
  sal::logger::worker_t,
  sal::logger::async_worker_t
>;


INSTANTIATE_TYPED_TEST_CASE_P(logger, channel, worker_types);


} // namespace
