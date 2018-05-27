#include <sal/service/application.hpp>
#include <sal/logger/logger.hpp>
#include <sal/common.test.hpp>
#include <sal/logger/common.test.hpp>
#include <cstdio>


namespace {


using namespace sal_test;


struct service_application
  : public sal_test::fixture
{
};


TEST_F(service_application, name)
{
  const char *argv[] =
  {
#if __sal_os_windows
    "app.exe",
#else
    "app",
#endif
  };

  sal::service::application_t app(1, argv, {});
  EXPECT_EQ("app", app.name);
#if __sal_os_windows
  EXPECT_EQ(".\\", app.path);
#else
  EXPECT_EQ("./", app.path);
#endif
}


TEST_F(service_application, name_with_path)
{
  const char *argv[] =
  {
#if __sal_os_windows
    "..\\..\\app.exe"
#else
    "../../app"
#endif
  };

  sal::service::application_t app(1, argv, {});
  EXPECT_EQ("app", app.name);
#if __sal_os_windows
  EXPECT_EQ("..\\..\\", app.path);
#else
  EXPECT_EQ("../../", app.path);
#endif
}


TEST_F(service_application, name_with_root)
{
  const char *argv[] =
  {
#if __sal_os_windows
    "\\app.exe"
#else
    "/app"
#endif
  };

  sal::service::application_t app(1, argv, {});
  EXPECT_EQ("app", app.name);
#if __sal_os_windows
  EXPECT_EQ("\\", app.path);
#else
  EXPECT_EQ("/", app.path);
#endif
}


TEST_F(service_application, no_name)
{
  EXPECT_THROW(
    sal::service::application_t app(0, {}, {}),
    std::exception
  );
}


TEST_F(service_application, add_options)
{
  using namespace sal::program_options;
  auto options = option_set_t().add({"version"}, help(case_name));

  const char *argv[] =
  {
    "app",
    "--version",
  };

  sal::service::application_t app(2, argv, options);
  std::ostringstream oss;
  app.print_help(oss);
  auto help = oss.str();

  EXPECT_NE(help.npos, help.find("app"));
  EXPECT_NE(help.npos, help.find("--version"));
  EXPECT_NE(help.npos, help.find(case_name));
}


TEST_F(service_application, add_reserved_options)
{
  using namespace sal::program_options;
  auto options = option_set_t().add({"help"}, help(case_name));

  const char *argv[] =
  {
    "app",
  };

  EXPECT_THROW(
    sal::service::application_t(1, argv, options),
    duplicate_option_name_error
  );
}


TEST_F(service_application, help)
{
  const char *argv[] =
  {
    "app",
    "--help",
  };
  sal::service::application_t app(2, argv, {});
  EXPECT_TRUE(app.help_requested());

  std::ostringstream oss;
  app.print_help(oss);
  auto help = oss.str();
  EXPECT_NE(help.npos, help.find("app"));
  EXPECT_NE(help.npos, help.find("help"));
}


TEST_F(service_application, no_help)
{
  const char *argv[] =
  {
    "app",
  };
  sal::service::application_t app(1, argv, {});
  EXPECT_FALSE(app.help_requested());
}


TEST_F(service_application, invalid_option)
{
  const char *argv[] =
  {
    "app",
    "--invalid",
  };
  EXPECT_THROW(
    sal::service::application_t(2, argv, {}),
    sal::program_options::unknown_option_error
  );
}


TEST_F(service_application, positional_argument)
{
  const char *argv[] =
  {
    "app",
    case_name.c_str(),
  };
  sal::service::application_t app(2, argv, {});

  auto &args = app.command_line.positional_arguments();
  ASSERT_EQ(1U, args.size());
  EXPECT_EQ(case_name, args[0]);
}


TEST_F(service_application, logger_stdout)
{
  std::ostringstream oss;
  auto rdbuf = std::cout.rdbuf(oss.rdbuf());
  {
    const char *argv[] =
    {
      "app",
      "--service.logger.sink=stdout",
    };
    sal::service::application_t app(2, argv, {});
    sal_log(app) << case_name;
  }
  std::cout.rdbuf(rdbuf);

  auto message = oss.str();
  EXPECT_NE(message.npos, message.find(case_name));
}


TEST_F(service_application, logger_null)
{
  std::ostringstream oss;
  auto rdbuf = std::cout.rdbuf(oss.rdbuf());
  {
    const char *argv[] =
    {
      "app",
      "--service.logger.sink=null",
    };
    sal::service::application_t app(2, argv, {});
    sal_log(app) << case_name;
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


TEST_F(service_application, logger_file)
{
  clean_logs("logs");

  {
    const char *argv[] =
    {
      "app",
      "--service.logger.dir=logs",
      "--service.logger.sink=app",
    };
    sal::service::application_t app(3, argv, {});
    sal_log(app) << case_name;
  }

  auto logs = directory_listing("logs");
  EXPECT_FALSE(logs.empty());
  EXPECT_TRUE(file_contains(case_name, logs[0]));

  clean_logs("logs");
}


} // namespace
