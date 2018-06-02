#include <sal/service/service.hpp>
#include <sal/common.test.hpp>


namespace {


using namespace sal_test;
using namespace std::chrono_literals;


struct service
  : public sal_test::fixture
{
  auto make_service ()
  {
    const char *argv[] =
    {
      "svc",
    };
    return sal::service::service_t<service>(std::size(argv), argv, {});
  }
};


TEST_F(service, run)
{
  auto svc = make_service();
  EXPECT_EQ(EXIT_SUCCESS, svc.run(1s));
}


} // namespace
