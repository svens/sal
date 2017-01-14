#include <sal/net/fwd.hpp>
#include <sal/common.test.hpp>


namespace {


using net = sal_test::fixture;


TEST_F(net, init)
{
  EXPECT_FALSE(bool(sal::net::init()));
}


} // namespace
