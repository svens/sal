#include <sal/logger/async_worker.hpp>
#include <sal/logger/logger.hpp>
#include <sal/logger/sink.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/common.test.hpp>


namespace {


template <typename Worker>
struct worker
  : public sal_test::with_type<Worker>
  , public sal::logger::sink_base_t
{
  void event_write (sal::logger::event_t &event) final override
  {
    (void)event;
  }
};


TYPED_TEST_CASE_P(worker);


TYPED_TEST_P(worker, default_logger)
{
  using namespace sal::logger;
}


TYPED_TEST_P(worker, get_logger)
{
  using namespace sal::logger;
}


TYPED_TEST_P(worker, make_logger)
{
  using namespace sal::logger;
}


REGISTER_TYPED_TEST_CASE_P(worker,
  default_logger,
  get_logger,
  make_logger
);


using worker_types = testing::Types<
  sal::logger::worker_t,
  sal::logger::async_worker_t
>;


INSTANTIATE_TYPED_TEST_CASE_P(logger, worker, worker_types);


} // namespace
