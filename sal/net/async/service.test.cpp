#include <sal/net/async/service.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_async_service
  : public sal_test::fixture
{
  sal::net::async::service_t service{10};
};


TEST_F(net_async_service, ctor)
{
  auto io = service.make_io(&service);
  EXPECT_EQ(&service, io.user_data());
}


} // namespace
