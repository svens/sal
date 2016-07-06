#include <sal/logger/logger.hpp>
#include <sal/logger/common.test.hpp>


namespace {


using namespace sal_test;
using logger = with_value<sal::logger::level_t>;


TEST_P(logger, print)
{
  using namespace sal::logger;

  worker_t loggers;
  auto logger = loggers.make_logger("first");
  (void)logger;
}


INSTANTIATE_TEST_CASE_P(logger, logger, logger_levels);


} // namespace
