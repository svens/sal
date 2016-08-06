#include <sal/logger/async_worker.hpp>
#include <sal/logger/channel.hpp>
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


TYPED_TEST_P(worker, default_channel_name)
{
  TypeParam worker;
  auto channel = worker.default_channel();
  EXPECT_EQ("", channel.name());
}


TYPED_TEST_P(worker, default_channel_is_enabled)
{
  TypeParam worker;

  auto channel = worker.default_channel();
  EXPECT_TRUE(channel.is_enabled());

  channel.set_enabled(false);
  EXPECT_FALSE(channel.is_enabled());

  channel.set_enabled(true);
  EXPECT_TRUE(channel.is_enabled());
}


TYPED_TEST_P(worker, default_channel_sink)
{
  auto sink = std::make_shared<sal_test::sink_t>();

  {
    TypeParam worker{set_channel_sink(sink)};
    auto channel = worker.default_channel();

    EXPECT_FALSE(sink->init_called);
    EXPECT_FALSE(sink->write_called);
    channel.make_event()->message << this->case_name;
  }

  EXPECT_TRUE(sink->init_called);
  EXPECT_TRUE(sink->write_called);
  EXPECT_TRUE(sink->last_message_contains(this->case_name));
}


TYPED_TEST_P(worker, get_channel_default)
{
  TypeParam worker;
  auto channel = worker.get_channel(this->case_name);
  EXPECT_EQ("", channel.name());
}


TYPED_TEST_P(worker, make_channel)
{
  auto sink = std::make_shared<sal_test::sink_t>();
  TypeParam worker{set_channel_sink(sink)};

  auto returned_channel = worker.make_channel(this->case_name);
  EXPECT_EQ(this->case_name, returned_channel.name());

  auto got_channel = worker.get_channel(this->case_name);
  EXPECT_EQ(this->case_name, got_channel.name());
}


inline bool ends_with (const std::string &name, const std::string &suffix)
{
  if (suffix.size() > name.size())
  {
    return false;
  }
  return std::equal(suffix.rbegin(), suffix.rend(), name.rbegin());
}


TYPED_TEST_P(worker, set_enabled_if)
{
  TypeParam worker;

  auto info = worker.make_channel(this->case_name);
  EXPECT_TRUE(info.is_enabled());
  auto debug = worker.make_channel(this->case_name + ".debug");
  EXPECT_TRUE(debug.is_enabled());

  worker.set_enabled_if(false,
    [](auto channel_name)
    {
      return ends_with(channel_name, ".debug");
    }
  );

  EXPECT_TRUE(info.is_enabled());
  EXPECT_FALSE(debug.is_enabled());
}


TYPED_TEST_P(worker, sink_throwing_event_init)
{
  auto sink = std::make_shared<sal_test::sink_t>();
  TypeParam worker{set_channel_sink(sink)};
  auto channel = worker.default_channel();

  sink->throw_init = true;
  EXPECT_EQ(nullptr, channel.make_event());
  EXPECT_TRUE(sink->init_called);
  EXPECT_FALSE(sink->write_called);
}


TYPED_TEST_P(worker, sink_throwing_event_write)
{
  auto sink = std::make_shared<sal_test::sink_t>();

  {
    TypeParam worker{set_channel_sink(sink)};
    auto channel = worker.default_channel();
    sink->throw_write = true;
    auto event = channel.make_event();
    ASSERT_NE(nullptr, event);
    event->message << this->case_name;
  }

  EXPECT_TRUE(sink->init_called);
  EXPECT_TRUE(sink->write_called);
  EXPECT_EQ("", sink->last_message);
}


REGISTER_TYPED_TEST_CASE_P(worker,
  default_channel_name,
  default_channel_is_enabled,
  default_channel_sink,
  get_channel_default,
  make_channel,
  set_enabled_if,
  sink_throwing_event_init,
  sink_throwing_event_write
);


using worker_types = testing::Types<
  sal::logger::worker_t,
  sal::logger::async_worker_t
>;


INSTANTIATE_TYPED_TEST_CASE_P(logger, worker, worker_types);


} // namespace
