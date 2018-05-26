#include <sal/service/service_base.hpp>
#include <sal/common.test.hpp>


namespace {


struct service_base
  : public sal_test::fixture
{
};


TEST_F(service_base, name)
{
  const char *argv[] =
  {
#if __sal_os_windows
    "application.exe",
#else
    "application",
#endif
  };

  sal::service::service_base_t svc(1, argv, {});
  EXPECT_EQ("application", svc.name);
#if __sal_os_windows
  EXPECT_EQ(".\\", svc.path);
#else
  EXPECT_EQ("./", svc.path);
#endif
}


TEST_F(service_base, name_with_path)
{
  const char *argv[] =
  {
#if __sal_os_windows
    "..\\..\\application.exe"
#else
    "../../application"
#endif
  };

  sal::service::service_base_t svc(1, argv, {});
  EXPECT_EQ("application", svc.name);
#if __sal_os_windows
  EXPECT_EQ("..\\..\\", svc.path);
#else
  EXPECT_EQ("../../", svc.path);
#endif
}


TEST_F(service_base, name_with_root)
{
  const char *argv[] =
  {
#if __sal_os_windows
    "\\application.exe"
#else
    "/application"
#endif
  };

  sal::service::service_base_t svc(1, argv, {});
  EXPECT_EQ("application", svc.name);
#if __sal_os_windows
  EXPECT_EQ("\\", svc.path);
#else
  EXPECT_EQ("/", svc.path);
#endif
}


TEST_F(service_base, no_name)
{
  EXPECT_THROW(
    sal::service::service_base_t svc(0, {}, {}),
    std::exception
  );
}


} // namespace
