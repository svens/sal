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
  TypeParam socket(TypeParam::protocol_t::v4());
  this->io_svc.associate(socket);
}


TYPED_TEST(net_io_service, associate_already_associated)
{
  TypeParam socket(TypeParam::protocol_t::v4());
  ASSERT_NO_THROW(this->io_svc.associate(socket));

  {
    std::error_code error;
    this->io_svc.associate(socket, error);
    EXPECT_EQ(sal::net::socket_errc::already_associated, error);
  }

  {
    EXPECT_THROW(this->io_svc.associate(socket), std::system_error);
  }
}


TYPED_TEST(net_io_service, associate_invalid_socket)
{
  TypeParam socket;

  {
    std::error_code error;
    this->io_svc.associate(socket, error);
    EXPECT_EQ(std::errc::invalid_argument, error);
  }

  {
    EXPECT_THROW(this->io_svc.associate(socket), std::system_error);
  }
}


} // namespace
