#include <sal/logger/level.hpp>
#include <sal/logger/common.test.hpp>


namespace {


using namespace sal_test;
using level = with_value<sal::logger::level_t>;


TEST_P(level, threshold)
{
  const sal::logger::level_t level = GetParam();

  {
    sal::logger::threshold_t threshold = level;
    EXPECT_TRUE(threshold.is_enabled(level));
    EXPECT_TRUE(threshold.is_enabled(less_verbose(level)));
    EXPECT_FALSE(threshold.is_enabled(more_verbose(level)));
  }

  {
    sal::logger::threshold_t threshold = less_verbose(level);
    EXPECT_FALSE(threshold.is_enabled(level));
    EXPECT_TRUE(threshold.is_enabled(less_verbose(level)));
    EXPECT_FALSE(threshold.is_enabled(more_verbose(level)));
  }

  {
    sal::logger::threshold_t threshold = more_verbose(level);
    EXPECT_TRUE(threshold.is_enabled(level));
    EXPECT_TRUE(threshold.is_enabled(less_verbose(level)));
    EXPECT_TRUE(threshold.is_enabled(more_verbose(level)));
  }
}


INSTANTIATE_TEST_CASE_P(logger, level, logger_levels);


} // namespace
