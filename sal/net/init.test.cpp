#include <sal/net/socket_base.hpp>
#include <sal/common.test.hpp>


namespace {


using net = sal_test::fixture;


TEST_F(net, init)
{
  auto first = sal::net::init();
  EXPECT_TRUE(!first);

  auto second = sal::net::init();
  EXPECT_TRUE(!second);

  EXPECT_EQ(first, second);
}


} // namespace
