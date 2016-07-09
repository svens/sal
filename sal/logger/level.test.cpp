#include <sal/logger/level.hpp>
#include <sal/logger/common.test.hpp>


namespace {


using namespace sal_test;
using level = with_value<sal::logger::level_t>;


TEST_P(level, threshold)
{
  const sal::logger::level_t l = GetParam();

  {
    sal::logger::threshold_t threshold = l;
    EXPECT_TRUE(threshold.is_enabled(l));
    EXPECT_TRUE(threshold.is_enabled(less_verbose(l)));
    EXPECT_FALSE(threshold.is_enabled(more_verbose(l)));
  }

  {
    sal::logger::threshold_t threshold = less_verbose(l);
    EXPECT_FALSE(threshold.is_enabled(l));
    EXPECT_TRUE(threshold.is_enabled(less_verbose(l)));
    EXPECT_FALSE(threshold.is_enabled(more_verbose(l)));
  }

  {
    sal::logger::threshold_t threshold = more_verbose(l);
    EXPECT_TRUE(threshold.is_enabled(l));
    EXPECT_TRUE(threshold.is_enabled(less_verbose(l)));
    EXPECT_TRUE(threshold.is_enabled(more_verbose(l)));
  }
}


INSTANTIATE_TEST_CASE_P(logger, level, logger_levels);


} // namespace
