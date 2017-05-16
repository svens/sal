#include <sal/logger/channel.hpp>
#include <sal/logger/async_worker.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>


namespace {


template <typename Worker>
struct logger_channel
  : public sal_test::with_type<Worker>
{
  std::shared_ptr<sal_test::sink_t> sink_{
    std::make_shared<sal_test::sink_t>()
  };

  std::unique_ptr<Worker> worker_{
    std::make_unique<Worker>(sal::logger::set_channel_sink(sink_))
  };

  sal::logger::channel_t<Worker> channel_{
    worker_->make_channel(this->case_name)
  };
};

using worker_types = testing::Types<
  sal::logger::worker_t,
  sal::logger::async_worker_t
>;

TYPED_TEST_CASE(logger_channel, worker_types);


TYPED_TEST(logger_channel, name)
{
  EXPECT_EQ(this->case_name, this->channel_.name());
}


TYPED_TEST(logger_channel, enabled)
{
  EXPECT_TRUE(this->channel_.is_enabled());

  this->channel_.set_enabled(false);
  EXPECT_FALSE(this->channel_.is_enabled());

  this->channel_.set_enabled(true);
  EXPECT_TRUE(this->channel_.is_enabled());
}


TYPED_TEST(logger_channel, make_event)
{
  EXPECT_FALSE(this->sink_->init_called);
  EXPECT_FALSE(this->sink_->write_called);
  {
    auto event = this->channel_.make_event();
    EXPECT_TRUE(this->sink_->init_called);
    this->sink_->init_called = false;
    EXPECT_FALSE(this->sink_->write_called);
  }

  // have to reset worker: async_worker will write out all queued events
  this->worker_.reset();

  EXPECT_FALSE(this->sink_->init_called);
  EXPECT_TRUE(this->sink_->write_called);
}


} // namespace
