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
  socket_t socket{{Address::loopback(), 0}}, test_socket{};


  void SetUp ()
  {
    socket.associate(service);
  }


  void connect ()
  {
    socket.connect(endpoint);
    test_socket = acceptor.accept();
  }


  void send (const std::string &data)
  {
    test_socket.send(data);
  }


  std::string receive ()
  {
    char buf[1024];
    return {buf, test_socket.receive(buf)};
  }


  void fill (sal::net::async::io_t &io, const std::string_view &data)
    noexcept
  {
    io.resize(data.size());
    std::memcpy(io.data(), data.data(), data.size());
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


//}}}1


template <typename Result>
inline std::string_view to_view (sal::net::async::io_t &io,
  const Result *result) noexcept
{
  return {reinterpret_cast<const char *>(io.data()), result->transferred};
}


TYPED_TEST(net_async_stream_socket, DISABLED_receive_async) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.receive_async(TestFixture::service.make_io());
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  ASSERT_EQ(nullptr, io.template get_if<socket_t::send_t>());

  auto result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_stream_socket, DISABLED_receive_async_after_send) //{{{1
{
  TestFixture::connect();

  TestFixture::send(TestFixture::case_name);
  TestFixture::socket.receive_async(TestFixture::service.make_io());

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  auto result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_stream_socket, DISABLED_receive_async_with_context) //{{{1
{
  TestFixture::connect();

  int socket_ctx = 1, io_ctx = 2;
  TestFixture::socket.context(&socket_ctx);

  TestFixture::socket.receive_async(TestFixture::service.make_io(&io_ctx));
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  EXPECT_EQ(&io_ctx, io.template context<int>());
  EXPECT_EQ(&socket_ctx, io.template socket_context<int>());

  auto result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_stream_socket, DISABLED_receive_async_canceled_on_close) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.receive_async(TestFixture::service.make_io());
  TestFixture::socket.close();

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_stream_socket, DISABLED_receive_async_no_sender) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.receive_async(TestFixture::service.make_io());
  EXPECT_TRUE(!TestFixture::worker.try_poll());
  EXPECT_TRUE(!TestFixture::worker.try_get());
  TestFixture::socket.close();

  auto io = TestFixture::worker.try_poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_stream_socket, DISABLED_receive_async_peek) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.receive_async(TestFixture::service.make_io(), socket_t::peek);
  TestFixture::send(TestFixture::case_name);

  // regardless of peek, completion should be removed from queue
  EXPECT_FALSE(!TestFixture::worker.poll());
  EXPECT_TRUE(!TestFixture::worker.try_poll());

  // but data must be still there
  TestFixture::socket.receive_async(TestFixture::service.make_io());
  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);
  auto result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_stream_socket, DISABLED_receive_async_peek_after_send) //{{{1
{
  TestFixture::connect();

  TestFixture::send(TestFixture::case_name);
  TestFixture::socket.receive_async(TestFixture::service.make_io(), socket_t::peek);

  // regardless of peek, completion should be removed from queue
  EXPECT_FALSE(!TestFixture::worker.poll());
  EXPECT_TRUE(!TestFixture::worker.try_poll());

  // but data must be still there
  TestFixture::socket.receive_async(TestFixture::service.make_io());
  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);
  auto result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_stream_socket, DISABLED_receive_async_less_than_send) //{{{1
{
  TestFixture::connect();

  auto io = TestFixture::service.make_io();
  io.resize(TestFixture::case_name.size() / 2);
  TestFixture::socket.receive_async(std::move(io));

  io = TestFixture::service.make_io();
  io.resize(TestFixture::case_name.size() - TestFixture::case_name.size() / 2);
  TestFixture::socket.receive_async(std::move(io));

  TestFixture::send(TestFixture::case_name);

  std::string data;
  for (auto i = 0;  i < 2;  ++i)
  {
    io = TestFixture::worker.poll();
    ASSERT_FALSE(!io);
    auto result = io.template get_if<socket_t::receive_t>();
    ASSERT_NE(nullptr, result);
    data += to_view(io, result);
  }
  EXPECT_EQ(TestFixture::case_name, data);
}


TYPED_TEST(net_async_stream_socket, DISABLED_receive_async_after_send_less_than_send) //{{{1
{
  TestFixture::connect();
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::service.make_io();
  io.resize(TestFixture::case_name.size() / 2);
  TestFixture::socket.receive_async(std::move(io));

  io = TestFixture::service.make_io();
  io.resize(TestFixture::case_name.size() - TestFixture::case_name.size() / 2);
  TestFixture::socket.receive_async(std::move(io));

  std::string data;
  for (auto i = 0;  i < 2;  ++i)
  {
    io = TestFixture::worker.poll();
    ASSERT_FALSE(!io);
    auto result = io.template get_if<socket_t::receive_t>();
    ASSERT_NE(nullptr, result);
    data += to_view(io, result);
  }
  EXPECT_EQ(TestFixture::case_name, data);
}


TYPED_TEST(net_async_stream_socket, DISABLED_receive_async_from_disconnected) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.receive_async(TestFixture::service.make_io());
  TestFixture::test_socket.close();

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred);
  EXPECT_TRUE(!error);
}


// }}}1


TYPED_TEST(net_async_stream_socket, DISABLED_send_async) //{{{1
{
  TestFixture::connect();

  auto io = TestFixture::service.make_io();
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.send_async(std::move(io));
  EXPECT_EQ(TestFixture::case_name, TestFixture::receive());

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  auto result = io.template get_if<socket_t::send_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name.size(), result->transferred);
}


TYPED_TEST(net_async_stream_socket, DISABLED_send_async_with_context) //{{{1
{
  TestFixture::connect();

  int socket_ctx = 1, io_ctx = 2;
  TestFixture::socket.context(&socket_ctx);

  auto io = TestFixture::service.make_io(&io_ctx);
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.send_async(std::move(io));
  EXPECT_EQ(TestFixture::case_name, TestFixture::receive());

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  EXPECT_EQ(&io_ctx, io.template context<int>());
  EXPECT_EQ(&socket_ctx, io.template socket_context<int>());

  auto result = io.template get_if<socket_t::send_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name.size(), result->transferred);
}


TYPED_TEST(net_async_stream_socket, DISABLED_send_async_not_connected) //{{{1
{
  auto io = TestFixture::service.make_io();
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.send_async(std::move(io));

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::send_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::not_connected, error);
}


TYPED_TEST(net_async_stream_socket, DISABLED_send_async_before_shutdown) //{{{1
{
  TestFixture::connect();

  auto io = TestFixture::service.make_io();
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.send_async(std::move(io));
  TestFixture::socket.shutdown(socket_t::shutdown_send);

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  auto result = io.template get_if<socket_t::send_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name.size(), result->transferred);
}


TYPED_TEST(net_async_stream_socket, DISABLED_send_async_after_shutdown) //{{{1
{
  TestFixture::connect();
  TestFixture::socket.shutdown(socket_t::shutdown_send);

  auto io = TestFixture::service.make_io();
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.send_async(std::move(io));

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::send_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::broken_pipe, error);
}


//}}}1


} // namespace
