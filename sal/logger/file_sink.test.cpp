#include <sal/logger/file_sink.hpp>
#include <sal/logger/async_worker.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/logger.hpp>
#include <sal/logger/common.test.hpp>


namespace {


template <typename Worker>
struct file_sink
  : public sal_test::with_type<Worker>
{
  sal::logger::sink_ptr sink_;
  Worker worker_;
  sal::logger::channel_t<Worker> channel_;

  file_sink ()
    : sink_(
        sal::logger::file("app",
          sal::logger::set_file_dir("test_logs"),
          sal::logger::set_file_buffer_size_kb(1),
          sal::logger::set_file_max_size_mb(1)
        )
      )
    , worker_(sal::logger::set_channel_sink(sink_))
    , channel_(worker_.make_channel(this->case_name))
  {}
};

TYPED_TEST_CASE_P(file_sink);


TYPED_TEST_P(file_sink, test)
{
  for (auto i = 0;  i < 10;  ++i)
  {
    sal_log(this->channel_) << i << " - yks";
    sal_log(this->channel_) << i << " - teine";
    sal_log(this->channel_) << i << " - kolmas";
  }
}


REGISTER_TYPED_TEST_CASE_P(file_sink,
  test
);


using worker_types = testing::Types<
  sal::logger::worker_t,
  sal::logger::async_worker_t
>;


INSTANTIATE_TYPED_TEST_CASE_P(logger, file_sink, worker_types);


} // namespace
