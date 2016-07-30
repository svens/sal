#include <sal/logger/sink.hpp>
#include <sal/logger/logger.hpp>
#include <sal/logger/async_worker.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>
#include <sstream>


namespace {


template <typename Worker>
struct ostream_sink
  : public sal_test::with_type<Worker>
{
  Worker worker_;


  ostream_sink ()
    : worker_()
  {}


  void test_ostream (sal::logger::sink_ptr sink, std::ostream &stream)
  {
    auto channel = worker_.make_channel("test_channel",
      sal::logger::set_channel_sink(sink)
    );

    // redirect logging to oss, log and reset back
    std::ostringstream oss;
    auto rdbuf = stream.rdbuf(oss.rdbuf());
    sal_log(channel) << this->case_name;
    stream.rdbuf(rdbuf);

    // check content
    auto content = oss.str();
    EXPECT_NE(content.npos, content.find("test_channel"));
    EXPECT_NE(content.npos, content.find(this->case_name));
  }


  void test_ostream_overflow (sal::logger::sink_ptr sink, std::ostream &stream)
  {
    auto channel = worker_.make_channel("test_channel",
      sal::logger::set_channel_sink(sink)
    );

    // redirect logging to oss, log and reset back
    std::ostringstream oss;
    auto rdbuf = stream.rdbuf(oss.rdbuf());
    std::string big_string(sal::logger::event_t::max_message_size, 'x');
    sal_log(channel) << this->case_name << ' ' << big_string;
    stream.rdbuf(rdbuf);

    // check content
    auto content = oss.str();
    EXPECT_NE(content.npos, content.find("test_channel"));
    EXPECT_NE(content.npos, content.find(this->case_name));
    EXPECT_NE(content.npos, content.find("<...>"));
  }
};

TYPED_TEST_CASE_P(ostream_sink);


TYPED_TEST_P(ostream_sink, cout)
{
  this->test_ostream(sal::logger::cout(), std::cout);
}


TYPED_TEST_P(ostream_sink, cerr)
{
  this->test_ostream(sal::logger::cerr(), std::cerr);
}


TYPED_TEST_P(ostream_sink, cout_overflow)
{
  this->test_ostream_overflow(sal::logger::cout(), std::cout);
}


TYPED_TEST_P(ostream_sink, cerr_overflow)
{
  this->test_ostream_overflow(sal::logger::cerr(), std::cerr);
}


REGISTER_TYPED_TEST_CASE_P(ostream_sink,
  cout,
  cerr,
  cout_overflow,
  cerr_overflow
);


using worker_types = testing::Types<
  sal::logger::worker_t,
  sal::logger::async_worker_t
>;


INSTANTIATE_TYPED_TEST_CASE_P(logger, ostream_sink, worker_types);


} // namespace
