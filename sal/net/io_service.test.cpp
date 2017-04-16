#include <sal/net/io_service.hpp>
#include <sal/net/internet.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_io_service
  : public sal_test::fixture
{
  using datagram_socket_t = sal::net::ip::udp_t::socket_t;
  using stream_socket_t = sal::net::ip::tcp_t::socket_t;
  using acceptor_t = sal::net::ip::tcp_t::acceptor_t;

  sal::net::io_service_t service;
};


TEST_F(net_io_service, make_context)
{
  auto ctx = service.make_context();
  (void)ctx;
}


TEST_F(net_io_service, make_context_too_small_completion_count)
{
  auto ctx = service.make_context(0);
  (void)ctx;
}


TEST_F(net_io_service, make_context_too_big_completion_count)
{
  auto ctx = service.make_context((std::numeric_limits<size_t>::max)());
  (void)ctx;
}


TEST_F(net_io_service, associate_datagram_socket)
{
  datagram_socket_t socket(sal::net::ip::udp_t::v4());
  std::error_code error;
  service.associate(socket, error);
  EXPECT_TRUE(!error);
}


TEST_F(net_io_service, associate_datagram_socket_multiple_times)
{
  datagram_socket_t socket(sal::net::ip::udp_t::v4());

  std::error_code error;
  service.associate(socket, error);
  EXPECT_TRUE(!error);

  service.associate(socket, error);
  EXPECT_FALSE(!error);
  EXPECT_EQ(sal::net::socket_errc_t::already_associated, error);

  EXPECT_THROW(service.associate(socket), std::system_error);
}


TEST_F(net_io_service, associate_datagram_socket_invalid)
{
  datagram_socket_t socket;

  std::error_code error;
  service.associate(socket, error);
  EXPECT_FALSE(!error);

  EXPECT_THROW(service.associate(socket), std::system_error);
}


TEST_F(net_io_service, associate_stream_socket)
{
  stream_socket_t socket(sal::net::ip::tcp_t::v4());
  std::error_code error;
  service.associate(socket, error);
  EXPECT_TRUE(!error);
}


TEST_F(net_io_service, associate_stream_socket_multiple_times)
{
  stream_socket_t socket(sal::net::ip::tcp_t::v4());

  std::error_code error;
  service.associate(socket, error);
  EXPECT_TRUE(!error);

  service.associate(socket, error);
  EXPECT_FALSE(!error);
  EXPECT_EQ(sal::net::socket_errc_t::already_associated, error);

  EXPECT_THROW(service.associate(socket), std::system_error);
}


TEST_F(net_io_service, associate_stream_socket_invalid)
{
  stream_socket_t socket;

  std::error_code error;
  service.associate(socket, error);
  EXPECT_FALSE(!error);

  EXPECT_THROW(service.associate(socket), std::system_error);
}


TEST_F(net_io_service, associate_acceptor_socket)
{
  acceptor_t acceptor(sal::net::ip::tcp_t::v4());
  std::error_code error;
  service.associate(acceptor, error);
  EXPECT_TRUE(!error);
}


TEST_F(net_io_service, associate_acceptor_socket_multiple_times)
{
  acceptor_t acceptor(sal::net::ip::tcp_t::v4());

  std::error_code error;
  service.associate(acceptor, error);
  EXPECT_TRUE(!error);

  service.associate(acceptor, error);
  EXPECT_FALSE(!error);
  EXPECT_EQ(sal::net::socket_errc_t::already_associated, error);

  EXPECT_THROW(service.associate(acceptor), std::system_error);
}


TEST_F(net_io_service, associate_acceptor_socket_invalid)
{
  acceptor_t socket;

  std::error_code error;
  service.associate(socket, error);
  EXPECT_FALSE(!error);

  EXPECT_THROW(service.associate(socket), std::system_error);
}


} // namespace
