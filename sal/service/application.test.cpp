#include <sal/service/application.hpp>
#include <sal/common.test.hpp>


namespace {


using namespace sal_test;


struct service_application
  : public sal_test::fixture
{
  template <size_t N>
  auto make_app (const char *(&&args)[N],
    sal::program_options::option_set_t options={})
  {
    return sal::service::application_t(
      static_cast<int>(std::size(args)), args, options
    );
  }
};


TEST_F(service_application, name)
{
  auto app = make_app({
#if __sal_os_windows
    "app.exe",
#else
    "app",
#endif
  });

  EXPECT_EQ("app", app.name);
#if __sal_os_windows
  EXPECT_EQ(".\\", app.path);
#else
  EXPECT_EQ("./", app.path);
#endif
}


TEST_F(service_application, name_with_path)
{
  auto app = make_app({
#if __sal_os_windows
    "..\\..\\app.exe"
#else
    "../../app"
#endif
  });

  EXPECT_EQ("app", app.name);
#if __sal_os_windows
  EXPECT_EQ("..\\..\\", app.path);
#else
  EXPECT_EQ("../../", app.path);
#endif
}


TEST_F(service_application, name_with_root)
{
  auto app = make_app({
#if __sal_os_windows
    "\\app.exe"
#else
    "/app"
#endif
  });

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

  auto app = make_app({"app", "--version"}, options);
  std::ostringstream oss;
  app.help(oss);
  auto help = oss.str();

  EXPECT_NE(help.npos, help.find("app"));
  EXPECT_NE(help.npos, help.find("--version"));
  EXPECT_NE(help.npos, help.find(case_name));
}


TEST_F(service_application, add_reserved_options)
{
  using namespace sal::program_options;
  auto options = option_set_t().add({"help"}, help(case_name));

  EXPECT_THROW(
    make_app({"app"}, options),
    duplicate_option_name_error
  );
}


TEST_F(service_application, help)
{
  auto app = make_app({"app", "--help"});
  EXPECT_TRUE(app.help_requested());

  std::ostringstream oss;
  EXPECT_EQ(EXIT_SUCCESS, app.help(oss));
  auto help = oss.str();
  EXPECT_NE(help.npos, help.find("app"));
  EXPECT_NE(help.npos, help.find("help"));
}


TEST_F(service_application, no_help)
{
  auto app = make_app({"app"});
  EXPECT_FALSE(app.help_requested());
}


TEST_F(service_application, invalid_option)
{
  EXPECT_THROW(
    make_app({"app", "--invalid"}),
    sal::program_options::unknown_option_error
  );
}


TEST_F(service_application, positional_argument)
{
  auto app = make_app({
    "app",
    case_name.c_str()
  });
  auto &args = app.command_line.positional_arguments();
  ASSERT_EQ(1U, args.size());
  EXPECT_EQ(case_name, args[0]);
}


} // namespace
