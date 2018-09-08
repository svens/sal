#include <sal/logger/sink.hpp>
#include <sal/logger/logger.hpp>
#include <sal/logger/async_worker.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>
#include <sstream>


namespace {


template <typename Worker>
struct logger_ostream_sink
  : public sal_test::with_type<Worker>
{
  void test_ostream (std::ostream &stream)
  {
    // redirect output to oss, log and restore
    std::ostringstream oss;
    auto rdbuf = stream.rdbuf(oss.rdbuf());
    {
      Worker worker;
      auto channel = worker.make_channel("test_channel",
        sal::logger::set_channel_sink(stream)
      );
      sal_log(channel) << this->case_name;
    }
    stream.rdbuf(rdbuf);

    // check content
    auto content = oss.str();
    EXPECT_NE(content.npos, content.find("test_channel"));
    EXPECT_NE(content.npos, content.find(this->case_name));
  }


  void test_ostream_overflow (std::ostream &stream)
  {
    // redirect output to oss, log and restore
    std::ostringstream oss;
    auto rdbuf = stream.rdbuf(oss.rdbuf());
    {
      Worker worker;
      auto channel = worker.make_channel("test_channel",
        sal::logger::set_channel_sink(stream)
      );
      std::string big_string(sal::logger::event_t::max_message_size, 'x');
      sal_log(channel) << this->case_name << ' ' << big_string;
    }
    stream.rdbuf(rdbuf);

    // check content
    auto content = oss.str();
    EXPECT_EQ(content.npos, content.find("test_channel"));
    EXPECT_EQ(content.npos, content.find(this->case_name));
    EXPECT_NE(content.npos, content.find("<...>"));
  }
};

using worker_types = testing::Types<
  sal::logger::worker_t,
  sal::logger::async_worker_t
>;

TYPED_TEST_CASE(logger_ostream_sink, worker_types, );


TYPED_TEST(logger_ostream_sink, cout)
{
  this->test_ostream(std::cout);
}


TYPED_TEST(logger_ostream_sink, cerr)
{
  this->test_ostream(std::cerr);
}


TYPED_TEST(logger_ostream_sink, cout_overflow)
{
  this->test_ostream_overflow(std::cout);
}


TYPED_TEST(logger_ostream_sink, cerr_overflow)
{
  this->test_ostream_overflow(std::cerr);
}


} // namespace
