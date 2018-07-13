#include <sal/net/async/basic_datagram_socket.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/common.test.hpp>
#include <cstring>


namespace {


using namespace std::chrono_literals;


template <typename Address>
struct net_async_datagram_socket
  : public sal_test::with_type<Address>
{
  static sal::net::ip::udp_t::endpoint_t endpoint () noexcept
  {
    return {Address::loopback(), 8195};
  }


  static sal::net::ip::udp_t protocol () noexcept
  {
    if constexpr (std::is_same_v<Address, sal::net::ip::address_v4_t>)
    {
      return sal::net::ip::udp_t::v4();
    }
    else
    {
      return sal::net::ip::udp_t::v6();
    }
  }


  sal::net::ip::udp_t::async_socket_t make_async_socket (
    size_t max_outstanding_receives = 1,
    size_t max_outstanding_sends = 1)
  {
    sal::net::ip::udp_t::async_socket_t socket(endpoint());
    socket.associate(service, max_outstanding_receives, max_outstanding_sends);
    return socket;
  }


  sal::net::ip::udp_t::socket_t make_test_socket ()
  {
    sal::net::ip::udp_t::socket_t socket(protocol());
    socket.connect(endpoint());
    return socket;
  }


  sal::net::async::io_t make_io ()
  {
    auto io = service.make_io();
    io.resize(this->case_name.size());
    std::memcpy(io.data(), this->case_name.data(), this->case_name.size());
    return io;
  }


  sal::net::async::service_t service{};
  sal::net::async::worker_t worker = service.make_worker(10);
  sal::net::ip::udp_t::socket_t test = make_test_socket();
  sal::net::ip::udp_t::endpoint_t test_endpoint = test.local_endpoint();
  char test_buf[1024] = { 0 };
};


using address_types = ::testing::Types<
  sal::net::ip::address_v4_t,
  sal::net::ip::address_v6_t
>;
TYPED_TEST_CASE(net_async_datagram_socket, address_types);


template <typename Result>
inline std::string_view to_view (sal::net::async::io_t &io,
  const Result *result) noexcept
{
  return {reinterpret_cast<const char *>(io.data()), result->transferred};
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_receive_from) //{{{1
{
  int io_context, socket_context;

  auto socket = TestFixture::make_async_socket();
  socket.context(&socket_context);

  socket.start_receive_from(TestFixture::service.make_io(&io_context));
  TestFixture::test.send(TestFixture::case_name);

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  auto ev = socket.receive_from_result(io);
  ASSERT_NE(nullptr, ev);

  EXPECT_EQ(TestFixture::case_name, to_view(io, ev));
  EXPECT_EQ(TestFixture::test.local_endpoint(), ev->remote_endpoint);

  EXPECT_EQ(&io_context, io.template context<int>());
  EXPECT_EQ(&socket_context, io.template socket_context<int>());
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_receive_from_after_send) //{{{1
{
  auto socket = TestFixture::make_async_socket();

  TestFixture::test.send(TestFixture::case_name);
  socket.start_receive_from(TestFixture::service.make_io());

  auto io = TestFixture::worker.poll(10ms);
  EXPECT_TRUE(!io);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_receive_from_canceled_on_close) //{{{1
{
  auto socket = TestFixture::make_async_socket();
  socket.start_receive_from(TestFixture::service.make_io());
  socket.close();

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto ev = socket.receive_from_result(io, error);
  ASSERT_NE(nullptr, ev);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_receive_from_with_closed_socket) //{{{1
{
  auto socket = TestFixture::make_async_socket();
  socket.close();
  socket.start_receive_from(TestFixture::service.make_io());

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto ev = socket.receive_from_result(io, error);
  ASSERT_NE(nullptr, ev);
  EXPECT_EQ(std::errc::bad_address, error);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_receive_from_no_sender) //{{{1
{
  int io_context, socket_context;

  {
    auto socket = TestFixture::make_async_socket();
    socket.context(&socket_context);
    socket.start_receive_from(TestFixture::service.make_io(&io_context));
    EXPECT_TRUE(!TestFixture::worker.try_poll());
    EXPECT_TRUE(!TestFixture::worker.try_get());
  }

  auto io = TestFixture::worker.try_poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto ev = sal::net::ip::udp_t::async_socket_t::receive_from_result(io, error);
  ASSERT_NE(nullptr, ev);
  EXPECT_EQ(std::errc::operation_canceled, error);

  EXPECT_EQ(&io_context, io.template context<int>());
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_receive_from_peek) //{{{1
{
  auto socket = TestFixture::make_async_socket();

  socket.start_receive_from(TestFixture::service.make_io(), socket.peek);
  TestFixture::test.send(TestFixture::case_name);

  // regardless of peek, completion should be removed from queue
  EXPECT_FALSE(!TestFixture::worker.poll());
  EXPECT_TRUE(!TestFixture::worker.try_poll());
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_receive_from_receive_less_than_send) //{{{1
{
  auto socket = TestFixture::make_async_socket();

  std::string_view data(
    TestFixture::case_name.data(),
    TestFixture::case_name.size() / 2
  );

  auto io = TestFixture::service.make_io();
  io.resize(data.size());
  socket.start_receive_from(std::move(io));
  TestFixture::test.send(TestFixture::case_name);

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto ev = socket.receive_from_result(io, error);
  ASSERT_NE(nullptr, ev);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(data.size(), ev->transferred);
  EXPECT_EQ(data, to_view(io, ev));
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_receive_from_receive_empty_buf) //{{{1
{
  auto socket = TestFixture::make_async_socket();

  auto io = TestFixture::service.make_io();
  io.resize(0);
  socket.start_receive_from(std::move(io));
  TestFixture::test.send(TestFixture::case_name);

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto ev = socket.receive_from_result(io, error);
  ASSERT_NE(nullptr, ev);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(0U, ev->transferred);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_receive_from_queue_overflow) //{{{1
{
  auto socket = TestFixture::make_async_socket();

  socket.start_receive_from(TestFixture::service.make_io());
  socket.start_receive_from(TestFixture::service.make_io());

  auto io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto ev = socket.receive_from_result(io, error);
  EXPECT_EQ(std::errc::no_buffer_space, error) << error.message();
  EXPECT_NE(nullptr, ev);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_send_to) //{{{1
{
  int io_context, socket_context;

  auto socket = TestFixture::make_async_socket();
  socket.context(&socket_context);

  auto io = TestFixture::make_io();
  io.context(&io_context);
  socket.start_send_to(std::move(io), TestFixture::test_endpoint);

  EXPECT_EQ(TestFixture::case_name.size(),
    TestFixture::test.receive(TestFixture::test_buf)
  );
  EXPECT_EQ(TestFixture::case_name, TestFixture::test_buf);

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  auto ev = socket.send_to_result(io);
  ASSERT_NE(nullptr, ev);
  EXPECT_EQ(TestFixture::case_name.size(), ev->transferred);

  EXPECT_EQ(&io_context, io.template context<int>());
  EXPECT_EQ(&socket_context, io.template socket_context<int>());
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_send_to_with_closed_socket) //{{{1
{
  auto socket = TestFixture::make_async_socket();
  socket.close();

  auto io = TestFixture::make_io();
  socket.start_send_to(std::move(io), TestFixture::test_endpoint);

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto ev = socket.send_to_result(io, error);
  ASSERT_NE(nullptr, ev);
  EXPECT_EQ(std::errc::bad_address, error);
}


TYPED_TEST(net_async_datagram_socket, DISABLED_start_send_to_empty_buf) //{{{1
{
  auto socket = TestFixture::make_async_socket();

  auto io = TestFixture::service.make_io();
  io.resize(0);
  socket.start_send_to(std::move(io), TestFixture::test_endpoint);

  EXPECT_EQ(0U, TestFixture::test.receive(TestFixture::test_buf));

  io = TestFixture::worker.poll();
  ASSERT_FALSE(!io);

  auto ev = socket.send_to_result(io);
  ASSERT_NE(nullptr, ev);
  EXPECT_EQ(0U, ev->transferred);
}


//}}}1


} // namespace
