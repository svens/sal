#include <sal/net/async/service.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/common.test.hpp>


namespace {


using socket_t = sal::net::ip::udp_t::socket_t;


template <typename Address>
struct net_async_datagram_socket
  : public sal_test::with_type<Address>
{
  const sal::net::ip::udp_t protocol =
    std::is_same_v<Address, sal::net::ip::address_v4_t>
      ? sal::net::ip::udp_t::v4()
      : sal::net::ip::udp_t::v6()
  ;

  const sal::net::ip::udp_t::endpoint_t endpoint{
    Address::loopback(),
    8195
  };

  sal::net::async::service_t service{};
  sal::net::async::worker_t worker = service.make_worker(2);
  socket_t socket{endpoint}, test_socket{protocol};


  void SetUp ()
  {
    socket.associate(service);
    test_socket.connect(endpoint);
  }


  void send (const std::string &data)
  {
    EXPECT_EQ(endpoint, test_socket.remote_endpoint());
    test_socket.send(data);
  }
};

using address_types = ::testing::Types<
  sal::net::ip::address_v4_t,
  sal::net::ip::address_v6_t
>;

TYPED_TEST_CASE(net_async_datagram_socket, address_types, );


template <typename Result>
inline std::string_view to_view (sal::net::async::io_t &io,
  const Result *result) noexcept
{
  return {reinterpret_cast<const char *>(io.data()), result->transferred};
}


TYPED_TEST(net_async_datagram_socket, DISABLED_receive_from_async) //{{{1
{
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  auto result = socket_t::receive_from_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
  EXPECT_EQ(TestFixture::test_socket.local_endpoint(), result->remote_endpoint);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_receive_from_async_after_send) //{{{1
{
  TestFixture::send(TestFixture::case_name);
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  auto result = socket_t::receive_from_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
  EXPECT_EQ(TestFixture::test_socket.local_endpoint(), result->remote_endpoint);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_receive_from_async_with_context) //{{{1
{
  int socket_ctx = 1, io_ctx = 2;
  TestFixture::socket.context(&socket_ctx);

  TestFixture::socket.receive_from_async(TestFixture::service.make_io(&io_ctx));
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  EXPECT_EQ(&io_ctx, io.template context<int>());
  EXPECT_EQ(&socket_ctx, io.template socket_context<int>());

  auto result = socket_t::receive_from_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
  EXPECT_EQ(TestFixture::test_socket.local_endpoint(), result->remote_endpoint);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_receive_from_async_canceled_on_close) //{{{1
{
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());
  TestFixture::socket.close();

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = socket_t::receive_from_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_receive_from_async_no_sender) //{{{1
{
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());
  EXPECT_TRUE(!TestFixture::worker.try_poll());
  EXPECT_TRUE(!TestFixture::worker.try_get());
  TestFixture::socket.close();

  auto io = TestFixture::worker.try_poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = socket_t::receive_from_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_receive_from_async_peek) //{{{1
{
  TestFixture::socket.receive_from_async(TestFixture::service.make_io(), socket_t::peek);
  TestFixture::send(TestFixture::case_name);

  // regardless of peek, completion should be removed from queue
  EXPECT_FALSE(!TestFixture::worker.poll());
  EXPECT_TRUE(!TestFixture::worker.try_poll());
}


TYPED_TEST(net_async_datagram_socket, DISABLED_receive_from_async_peek_after_send) //{{{1
{
  TestFixture::send(TestFixture::case_name);
  TestFixture::socket.receive_from_async(TestFixture::service.make_io(), socket_t::peek);

  // regardless of peek, completion should be removed from queue
  EXPECT_FALSE(!TestFixture::worker.poll());
  EXPECT_TRUE(!TestFixture::worker.try_poll());
}


TYPED_TEST(net_async_datagram_socket, DISABLED_receive_from_async_less_than_send) //{{{1
{
  std::string_view data{
    TestFixture::case_name.data(),
    TestFixture::case_name.size() / 2,
  };

  auto io = TestFixture::service.make_io();
  io.resize(data.size());
  TestFixture::socket.receive_from_async(std::move(io));
  TestFixture::send(TestFixture::case_name);

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = socket_t::receive_from_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(data.size(), result->transferred);
  EXPECT_EQ(data, to_view(io, result));
}


TYPED_TEST(net_async_datagram_socket, DISABLED_receive_from_async_after_send_less_than_send) //{{{1
{
  TestFixture::send(TestFixture::case_name);

  std::string_view data{
    TestFixture::case_name.data(),
    TestFixture::case_name.size() / 2,
  };

  auto io = TestFixture::service.make_io();
  io.resize(data.size());
  TestFixture::socket.receive_from_async(std::move(io));

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = socket_t::receive_from_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(data.size(), result->transferred);
  EXPECT_EQ(data, to_view(io, result));
}


TYPED_TEST(net_async_datagram_socket, DISABLED_receive_from_async_empty_buf) //{{{1
{
  auto io = TestFixture::service.make_io();
  io.resize(0);
  TestFixture::socket.receive_from_async(std::move(io));
  TestFixture::send(TestFixture::case_name);

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = socket_t::receive_from_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(0U, result->transferred);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_receive_from_async_after_send_empty_buf) //{{{1
{
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::service.make_io();
  io.resize(0);
  TestFixture::socket.receive_from_async(std::move(io));

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = socket_t::receive_from_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(0U, result->transferred);
}


//}}}1


} // namespace
