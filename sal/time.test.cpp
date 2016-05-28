#include <sal/time.hpp>
#include <sal/common.test.hpp>


#if __sal_os_windows
  #define timegm _mkgmtime
  #define putenv _putenv
  #define tzset _tzset
#endif


namespace {


TEST(time, local_time)
{
  std::time_t now = sal::system_clock::to_time_t(sal::now());
  std::tm tm = sal::local_time(now);
  EXPECT_NE(0, tm.tm_mday);

  std::time_t now1 = std::mktime(&tm);
  EXPECT_EQ(now, now1);
}


TEST(time, utc_time)
{
  std::time_t now = sal::system_clock::to_time_t(sal::now());
  std::tm tm = sal::utc_time(now);
  EXPECT_NE(0, tm.tm_mday);

  std::time_t now1 = timegm(&tm);
  EXPECT_EQ(now, now1);
}


TEST(time, local_offset)
{
  for (int h = -12;  h <= 14;  ++h)
  {
    char zone[sizeof("TZ=GMT+HH")];
    (void)std::snprintf(zone, sizeof(zone), "TZ=GMT%+02d", h);
    (void)putenv(zone);
    tzset();

    auto offset = sal::local_offset(sal::now());
    EXPECT_EQ(-h*60*60, offset.count());
  }
}


} // namespace
