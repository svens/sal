#include <sal/net/io_service.hpp>
#include <sal/net/internet.hpp>
#include <sal/common.test.hpp>


namespace {


template <typename Socket>
struct net_io_service
  : public sal_test::with_type<Socket>
{
  sal::net::io_service_t io_svc;
};

using types = ::testing::Types<
  sal::net::ip::udp_t::socket_t,
  sal::net::ip::tcp_t::socket_t,
  sal::net::ip::tcp_t::acceptor_t
>;

TYPED_TEST_CASE(net_io_service, types);


TYPED_TEST(net_io_service, associate)
{
  /*
  TypeParam socket;
  EXPECT_FALSE(socket.is_associated());
  */
}


} // namespace
