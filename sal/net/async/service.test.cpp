#include <sal/net/async/service.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_async_service
  : public sal_test::fixture
{
  sal::net::async::service_t service{};
};


TEST_F(net_async_service, ctor)
{
  auto context = service.make_context();
  auto io = context.make_io(&service);
  EXPECT_EQ(nullptr, io.user_data<net_async_service>());
  EXPECT_NE(nullptr, io.user_data<sal::net::async::service_t>());
}


} // namespace
