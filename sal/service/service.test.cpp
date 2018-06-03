#include <sal/service/service.hpp>
#include <sal/common.test.hpp>


namespace {


using namespace sal_test;
using namespace std::chrono_literals;


struct service
  : public sal_test::fixture
{
  template <size_t N>
  auto make_service (const char *(&&args)[N])
  {
    return sal::service::service_t<service>(
      static_cast<int>(std::size(args)),
      args,
      {}
    );
  }

  auto make_service ()
  {
    return make_service({"svc"});
  }
};


TEST_F(service, run)
{
  auto svc = make_service();
  EXPECT_EQ(EXIT_SUCCESS, svc.run(1s));
}


} // namespace
