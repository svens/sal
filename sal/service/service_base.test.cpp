#include <sal/service/service_base.hpp>
#include <sal/logger/logger.hpp>
#include <sal/common.test.hpp>
#include <sal/logger/common.test.hpp>
#include <cstdio>


namespace {


using namespace sal_test;


struct service_base
  : public sal_test::fixture
{ };


TEST_F(service_base, help)
{
  const char *argv[] =
  {
    "svc",
    "--help",
  };
  sal::service::service_base_t svc(2, argv, {});
  EXPECT_TRUE(svc.help_requested());

  std::ostringstream oss;
  EXPECT_EQ(EXIT_SUCCESS, svc.help(oss));
  auto help = oss.str();
  EXPECT_NE(help.npos, help.find("svc"));
  EXPECT_NE(help.npos, help.find("help"));
  EXPECT_NE(help.npos, help.find("service.logger.dir"));
  EXPECT_NE(help.npos, help.find("service.logger.sink"));
}


TEST_F(service_base, logger_stdout)
{
  std::ostringstream oss;
  auto rdbuf = std::cout.rdbuf(oss.rdbuf());
  {
    const char *argv[] =
    {
      "svc",
      "--service.logger.sink=stdout",
    };
    sal::service::service_base_t svc(2, argv, {});
    sal_log(svc) << case_name;
  }
  std::cout.rdbuf(rdbuf);

  auto message = oss.str();
  EXPECT_NE(message.npos, message.find(case_name));
}


TEST_F(service_base, logger_null)
{
  std::ostringstream oss;
  auto rdbuf = std::cout.rdbuf(oss.rdbuf());
  {
    const char *argv[] =
    {
      "svc",
      "--service.logger.sink=null",
    };
    sal::service::service_base_t svc(2, argv, {});
    sal_log(svc) << case_name;
  }
  std::cout.rdbuf(rdbuf);

  EXPECT_TRUE(oss.str().empty());
}


void clean_logs (const std::string &dir)
{
  for (auto &file: directory_listing(dir))
  {
    std::remove(file.c_str());
  }
#if __sal_os_windows
  ::_rmdir(dir.c_str());
#else
  ::rmdir(dir.c_str());
#endif
}


TEST_F(service_base, logger_file)
{
  clean_logs("logs");

  {
    const char *argv[] =
    {
      "svc",
      "--service.logger.dir=logs",
      "--service.logger.sink=svc",
    };
    sal::service::service_base_t svc(3, argv, {});
    sal_log(svc) << case_name;
  }

  auto logs = directory_listing("logs");
  EXPECT_FALSE(logs.empty());
  EXPECT_TRUE(file_contains(case_name, logs[0]));

  clean_logs("logs");
}


} // namespace
