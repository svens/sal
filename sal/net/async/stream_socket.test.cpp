#include <sal/net/async/service.hpp>
#include <sal/net/ip/tcp.hpp>
#include <sal/common.test.hpp>


namespace {


using socket_t = sal::net::ip::tcp_t::socket_t;
using acceptor_t = sal::net::ip::tcp_t::acceptor_t;


template <typename Address>
struct net_async_stream_socket
  : public sal_test::with_type<Address>
{
  const sal::net::ip::tcp_t protocol =
    std::is_same_v<Address, sal::net::ip::address_v4_t>
      ? sal::net::ip::tcp_t::v4()
      : sal::net::ip::tcp_t::v6()
  ;

  const sal::net::ip::tcp_t::endpoint_t endpoint{
    Address::loopback(),
    8195
  };

  sal::net::async::service_t service{};
  sal::net::async::worker_t worker = service.make_worker(2);
  acceptor_t acceptor{endpoint};
  socket_t socket{{Address::loopback(), 0}};


  void SetUp ()
  {
    socket.associate(service);
  }
};

using address_types = ::testing::Types<
  sal::net::ip::address_v4_t,
  sal::net::ip::address_v6_t
>;

TYPED_TEST_CASE(net_async_stream_socket, address_types, );


TYPED_TEST(net_async_stream_socket, DISABLED_connect_async) //{{{1
{
  TestFixture::socket.connect_async(
    TestFixture::service.make_io(),
    TestFixture::endpoint
  );

  auto a = TestFixture::acceptor.accept();

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  EXPECT_EQ(nullptr, io.template get_if<acceptor_t::accept_t>());

  auto result = io.template get_if<socket_t::connect_t>();
  ASSERT_NE(nullptr, result);

  EXPECT_EQ(a.local_endpoint(), TestFixture::socket.remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), TestFixture::socket.local_endpoint());
}


TYPED_TEST(net_async_stream_socket, DISABLED_connect_async_refused) //{{{1
{
  auto echo_endpoint = TestFixture::endpoint;
  echo_endpoint.port(7);

  TestFixture::socket.connect_async(
    TestFixture::service.make_io(),
    echo_endpoint
  );

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::connect_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::connection_refused, error);
}


TYPED_TEST(net_async_stream_socket, DISABLED_connect_async_already_connected) //{{{1
{
  TestFixture::socket.connect(TestFixture::endpoint);
  auto a = TestFixture::acceptor.accept();

  auto echo_endpoint = TestFixture::endpoint;
  echo_endpoint.port(7);

  TestFixture::socket.connect_async(
    TestFixture::service.make_io(),
    echo_endpoint
  );

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::connect_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::already_connected, error);
}


TYPED_TEST(net_async_stream_socket, DISABLED_connect_async_address_not_available) //{{{1
{
  // connect from loopback to any
  TestFixture::socket.connect_async(
    TestFixture::service.make_io(),
    {TypeParam::any(), TestFixture::endpoint.port()}
  );

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::connect_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::address_not_available, error);
}


// }}}1


} // namespace
