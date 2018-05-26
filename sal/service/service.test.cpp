#include <sal/service/service.hpp>
#include <sal/common.test.hpp>


namespace {


struct service
  : public sal_test::fixture
{
  using test_service_t = sal::service::service_t<service>;

  const char *argv[1] =
  {
    "app",
  };
  const int argc = sizeof(argv) / sizeof(argv[0]);
};


static auto options ()
{
  using namespace sal::program_options;
  return option_set_t()
    .add({"version", "v"},
      help("print version and exit")
    );
}


TEST_F(service, name)
{
  test_service_t svc(argc, argv, options());
  EXPECT_EQ("app", svc.name);
}


} // namespace
