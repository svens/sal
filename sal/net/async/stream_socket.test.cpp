#include <sal/net/async/completion_queue.hpp>
#include <sal/net/async/service.hpp>
#include <sal/net/ip/tcp.hpp>
#include <sal/net/common.test.hpp>
#include <thread>


namespace {


using namespace std::chrono_literals;
using sal_test::to_view;

using socket_t = sal::net::ip::tcp_t::socket_t;
using acceptor_t = sal::net::ip::tcp_t::acceptor_t;


template <typename Address>
struct net_async_stream_socket
  : public sal_test::with_type<Address>
{
  static constexpr bool with_ipv4 =
    std::is_same_v<Address, sal::net::ip::address_v4_t>;

  const sal::net::ip::tcp_t protocol = with_ipv4
      ? sal::net::ip::tcp_t::v4
      : sal::net::ip::tcp_t::v6
  ;

  const sal::net::ip::tcp_t::endpoint_t endpoint{
    Address::loopback,
    8195
  };

  const sal::net::ip::tcp_t::endpoint_t not_supported_family_endpoint{
    with_ipv4 ? sal::net::ip::tcp_t::v6 : sal::net::ip::tcp_t::v4,
    endpoint.port()
  };

  sal::net::async::service_t service{};
  sal::net::async::completion_queue_t queue{service};
  acceptor_t acceptor{endpoint};
  socket_t socket{{Address::loopback, 0}}, test_socket{};


  void SetUp ()
  {
    socket.associate(service);
  }


  void connect ()
  {
    socket.connect(endpoint);
    test_socket = acceptor.accept();
  }


  void disconnect ()
  {
    test_socket.close();
    std::this_thread::sleep_for(10ms);
  }


  sal::net::async::io_ptr wait ()
  {
    auto io = queue.try_get();
    if (!io && queue.wait())
    {
      io = queue.try_get();
    }
    return io;
  }


  sal::net::async::io_ptr poll ()
  {
    auto io = queue.try_get();
    if (!io && queue.poll())
    {
      io = queue.try_get();
    }
    return io;
  }


  void send (const std::string &data, bool pause_after_send = false)
  {
    test_socket.send(data);
    if (pause_after_send)
    {
      std::this_thread::sleep_for(1ms);
    }
  }


  std::string receive ()
  {
    char buf[1024];
    return {buf, test_socket.receive(buf)};
  }


  void fill (sal::net::async::io_ptr &io, const std::string_view &data)
    noexcept
  {
    io->resize(data.size());
    std::memcpy(io->data(), data.data(), data.size());
  }
};

TYPED_TEST_CASE(net_async_stream_socket,
  sal_test::address_types,
  sal_test::address_names
);


TYPED_TEST(net_async_stream_socket, start_connect) //{{{1
{
  TestFixture::socket.start_connect(
    TestFixture::queue.make_io(),
    TestFixture::endpoint
  );

  auto a = TestFixture::acceptor.accept();

  auto io = TestFixture::poll();
  ASSERT_NE(nullptr, io);

  EXPECT_EQ(nullptr, io->template get_if<acceptor_t::accept_t>());

  auto result = io->template get_if<socket_t::connect_t>();
  ASSERT_NE(nullptr, result);

  EXPECT_EQ(a.local_endpoint(), TestFixture::socket.remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), TestFixture::socket.local_endpoint());
}


TYPED_TEST(net_async_stream_socket, start_connect_without_associate) //{{{1
{
  if (sal::is_debug_build)
  {
    socket_t s;
    EXPECT_THROW(
      s.start_connect(TestFixture::queue.make_io(), TestFixture::endpoint),
      std::logic_error
    );
  }
}


TYPED_TEST(net_async_stream_socket, start_connect_non_blocking) //{{{1
{
  TestFixture::socket.non_blocking(true);
  TestFixture::socket.start_connect(
    TestFixture::queue.make_io(),
    TestFixture::endpoint
  );

  auto a = TestFixture::acceptor.accept();

  auto io = TestFixture::poll();
  ASSERT_NE(nullptr, io);

  auto result = io->template get_if<socket_t::connect_t>();
  ASSERT_NE(nullptr, result);

  EXPECT_EQ(a.local_endpoint(), TestFixture::socket.remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), TestFixture::socket.local_endpoint());
}


TYPED_TEST(net_async_stream_socket, start_connect_with_context) //{{{1
{
  int socket_ctx = 1, io_ctx = 2;
  TestFixture::socket.context(&socket_ctx);

  TestFixture::socket.start_connect(
    TestFixture::service.make_io(&io_ctx),
    TestFixture::endpoint
  );

  auto a = TestFixture::acceptor.accept();

  auto io = TestFixture::poll();
  ASSERT_NE(nullptr, io);

  EXPECT_EQ(&io_ctx, io->template context<int>());
  EXPECT_EQ(nullptr, io->template context<socket_t>());
  EXPECT_EQ(&socket_ctx, io->template socket_context<int>());
  EXPECT_EQ(nullptr, io->template socket_context<socket_t>());

  auto result = io->template get_if<socket_t::connect_t>();
  ASSERT_NE(nullptr, result);

  EXPECT_EQ(a.local_endpoint(), TestFixture::socket.remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), TestFixture::socket.local_endpoint());
}


TYPED_TEST(net_async_stream_socket, start_connect_refused) //{{{1
{
  auto echo_endpoint = TestFixture::endpoint;
  echo_endpoint.port(7);

  TestFixture::socket.start_connect(
    TestFixture::queue.make_io(),
    echo_endpoint
  );

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::connect_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::connection_refused, error);
}


TYPED_TEST(net_async_stream_socket, start_connect_non_blocking_refused) //{{{1
{
  auto echo_endpoint = TestFixture::endpoint;
  echo_endpoint.port(7);

  TestFixture::socket.non_blocking(true);
  TestFixture::socket.start_connect(
    TestFixture::queue.make_io(),
    echo_endpoint
  );

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::connect_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::connection_refused, error);
}


TYPED_TEST(net_async_stream_socket, start_connect_already_connected) //{{{1
{
  TestFixture::socket.connect(TestFixture::endpoint);
  auto a = TestFixture::acceptor.accept();

  auto echo_endpoint = TestFixture::endpoint;
  echo_endpoint.port(7);

  TestFixture::socket.start_connect(
    TestFixture::queue.make_io(),
    echo_endpoint
  );

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::connect_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::already_connected, error);
}


TYPED_TEST(net_async_stream_socket, start_connect_non_blocking_already_connected) //{{{1
{
  TestFixture::socket.connect(TestFixture::endpoint);
  auto a = TestFixture::acceptor.accept();

  auto echo_endpoint = TestFixture::endpoint;
  echo_endpoint.port(7);

  TestFixture::socket.non_blocking(true);
  TestFixture::socket.start_connect(
    TestFixture::queue.make_io(),
    echo_endpoint
  );

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::connect_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::already_connected, error);
}


TYPED_TEST(net_async_stream_socket, start_connect_address_family_not_supported) //{{{1
{
  // connect from loopback to any
  TestFixture::socket.start_connect(
    TestFixture::queue.make_io(),
    TestFixture::not_supported_family_endpoint
  );

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::connect_t>(error);
  ASSERT_NE(nullptr, result);

#if __sal_os_linux
  if constexpr (TestFixture::with_ipv4)
  {
    EXPECT_EQ(std::errc::address_family_not_supported, error);
  }
  else
  {
    EXPECT_EQ(std::errc::invalid_argument, error);
  }
#else
  EXPECT_EQ(std::errc::address_family_not_supported, error);
#endif
}


TYPED_TEST(net_async_stream_socket, start_connect_non_blocking_address_family_not_supported) //{{{1
{
  TestFixture::socket.non_blocking(true);

  // connect from loopback to any
  TestFixture::socket.start_connect(
    TestFixture::queue.make_io(),
    TestFixture::not_supported_family_endpoint
  );

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::connect_t>(error);
  ASSERT_NE(nullptr, result);

#if __sal_os_linux
  if constexpr (TestFixture::with_ipv4)
  {
    EXPECT_EQ(std::errc::address_family_not_supported, error);
  }
  else
  {
    EXPECT_EQ(std::errc::invalid_argument, error);
  }
#else
  EXPECT_EQ(std::errc::address_family_not_supported, error);
#endif
}


//}}}1


TYPED_TEST(net_async_stream_socket, start_receive) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.start_receive(TestFixture::queue.make_io());
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  ASSERT_EQ(nullptr, io->template get_if<socket_t::send_t>());

  auto result = io->template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_stream_socket, start_receive_without_associate) //{{{1
{
  if (sal::is_debug_build)
  {
    socket_t s;
    s.connect(TestFixture::endpoint);
    TestFixture::test_socket = TestFixture::acceptor.accept();
    EXPECT_THROW(
      s.start_receive(TestFixture::queue.make_io()),
      std::logic_error
    );
  }
}


TYPED_TEST(net_async_stream_socket, start_receive_after_send) //{{{1
{
  TestFixture::connect();

  TestFixture::send(TestFixture::case_name);
  TestFixture::socket.start_receive(TestFixture::queue.make_io());

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  auto result = io->template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_stream_socket, start_receive_with_context) //{{{1
{
  TestFixture::connect();

  int socket_ctx = 1, io_ctx = 2;
  TestFixture::socket.context(&socket_ctx);

  TestFixture::socket.start_receive(TestFixture::service.make_io(&io_ctx));
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  EXPECT_EQ(&io_ctx, io->template context<int>());
  EXPECT_EQ(nullptr, io->template context<socket_t>());
  EXPECT_EQ(&socket_ctx, io->template socket_context<int>());
  EXPECT_EQ(nullptr, io->template socket_context<socket_t>());

  auto result = io->template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_stream_socket, start_receive_two_sends) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.start_receive(TestFixture::queue.make_io());
  TestFixture::socket.start_receive(TestFixture::queue.make_io());
  TestFixture::send(TestFixture::case_name);
  TestFixture::send(TestFixture::case_name, true);

  for (auto i = 0U;  i < 2;  ++i)
  {
    auto io = TestFixture::wait();
    ASSERT_NE(nullptr, io);

    auto result = io->template get_if<socket_t::receive_t>();
    ASSERT_NE(nullptr, result);

    if (TestFixture::case_name.size() == result->transferred)
    {
      // got one receive, wait for another
      EXPECT_EQ(TestFixture::case_name, to_view(io, result));
    }
    else
    {
      // got both receives, we are done
      EXPECT_EQ(
        TestFixture::case_name + TestFixture::case_name,
        to_view(io, result)
      );
      break;
    }
  }
}


TYPED_TEST(net_async_stream_socket, start_receive_after_two_sends) //{{{1
{
  TestFixture::connect();

  TestFixture::send(TestFixture::case_name);
  TestFixture::send(TestFixture::case_name, true);
  TestFixture::socket.start_receive(TestFixture::queue.make_io());

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  auto result = io->template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(
    TestFixture::case_name + TestFixture::case_name,
    to_view(io, result)
  );
}


TYPED_TEST(net_async_stream_socket, start_receive_canceled_on_close) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.start_receive(TestFixture::queue.make_io());
  TestFixture::socket.close();

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_stream_socket, start_receive_no_sender) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.start_receive(TestFixture::queue.make_io());
  EXPECT_FALSE(TestFixture::poll());
  EXPECT_FALSE(TestFixture::queue.try_get());
  TestFixture::socket.close();

  auto io = TestFixture::poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_stream_socket, start_receive_peek) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.start_receive(TestFixture::queue.make_io(), socket_t::peek);
  TestFixture::send(TestFixture::case_name);

  // regardless of peek, completion should be removed from queue
  EXPECT_TRUE(TestFixture::wait());
  EXPECT_FALSE(TestFixture::poll());

  // but data must be still there
  TestFixture::socket.start_receive(TestFixture::queue.make_io());
  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);
  auto result = io->template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_stream_socket, start_receive_peek_after_send) //{{{1
{
  TestFixture::connect();

  TestFixture::send(TestFixture::case_name);
  TestFixture::socket.start_receive(TestFixture::queue.make_io(), socket_t::peek);

  // regardless of peek, completion should be removed from queue
  EXPECT_TRUE(TestFixture::wait());
  EXPECT_FALSE(TestFixture::poll());

  // but data must be still there
  TestFixture::socket.start_receive(TestFixture::queue.make_io());
  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);
  auto result = io->template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_stream_socket, start_receive_less_than_send) //{{{1
{
  TestFixture::connect();

  auto io = TestFixture::queue.make_io();
  io->resize(TestFixture::case_name.size() / 2);
  TestFixture::socket.start_receive(std::move(io));

  io = TestFixture::queue.make_io();
  io->resize(TestFixture::case_name.size() - TestFixture::case_name.size() / 2);
  TestFixture::socket.start_receive(std::move(io));

  TestFixture::send(TestFixture::case_name);

  std::string data;
  for (auto i = 0;  i < 2;  ++i)
  {
    io = TestFixture::wait();
    ASSERT_NE(nullptr, io);
    auto result = io->template get_if<socket_t::receive_t>();
    ASSERT_NE(nullptr, result);
    data += to_view(io, result);
  }
  EXPECT_EQ(TestFixture::case_name, data);
}


TYPED_TEST(net_async_stream_socket, start_receive_after_send_less_than_send) //{{{1
{
  TestFixture::connect();
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::queue.make_io();
  io->resize(TestFixture::case_name.size() / 2);
  TestFixture::socket.start_receive(std::move(io));

  io = TestFixture::queue.make_io();
  io->resize(TestFixture::case_name.size() - TestFixture::case_name.size() / 2);
  TestFixture::socket.start_receive(std::move(io));

  std::string data;
  for (auto i = 0;  i < 2;  ++i)
  {
    io = TestFixture::wait();
    ASSERT_NE(nullptr, io);
    auto result = io->template get_if<socket_t::receive_t>();
    ASSERT_NE(nullptr, result);
    data += to_view(io, result);
  }
  EXPECT_EQ(TestFixture::case_name, data);
}


TYPED_TEST(net_async_stream_socket, start_receive_from_disconnected) //{{{1
{
  TestFixture::connect();
  TestFixture::socket.start_receive(TestFixture::queue.make_io());
  TestFixture::disconnect();

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred);
  EXPECT_EQ(std::errc::broken_pipe, error);
}


TYPED_TEST(net_async_stream_socket, start_receive_after_from_disconnected) //{{{1
{
  TestFixture::connect();
  TestFixture::disconnect();
  TestFixture::socket.start_receive(TestFixture::queue.make_io());

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred);
  EXPECT_EQ(std::errc::broken_pipe, error);
}


TYPED_TEST(net_async_stream_socket, start_receive_before_shutdown) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.start_receive(TestFixture::queue.make_io());
  TestFixture::socket.shutdown(socket_t::shutdown_receive);

  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);

#if __sal_os_macos
  EXPECT_EQ(std::errc::broken_pipe, error);
#else
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
#endif
}


TYPED_TEST(net_async_stream_socket, start_receive_after_shutdown) //{{{1
{
  TestFixture::connect();

  TestFixture::socket.shutdown(socket_t::shutdown_receive);
  TestFixture::socket.start_receive(TestFixture::queue.make_io());

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::broken_pipe, error);
}


// }}}1


TYPED_TEST(net_async_stream_socket, start_send) //{{{1
{
  TestFixture::connect();

  auto io = TestFixture::queue.make_io();
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.start_send(std::move(io));
  EXPECT_EQ(TestFixture::case_name, TestFixture::receive());

  io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  auto result = io->template get_if<socket_t::send_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name.size(), result->transferred);
}


TYPED_TEST(net_async_stream_socket, start_send_without_associate) //{{{1
{
  if (sal::is_debug_build)
  {
    socket_t s;
    s.connect(TestFixture::endpoint);
    TestFixture::test_socket = TestFixture::acceptor.accept();
    EXPECT_THROW(
      s.start_send(TestFixture::queue.make_io()),
      std::logic_error
    );
  }
}


TYPED_TEST(net_async_stream_socket, start_send_with_context) //{{{1
{
  TestFixture::connect();

  int socket_ctx = 1, io_ctx = 2;
  TestFixture::socket.context(&socket_ctx);

  auto io = TestFixture::service.make_io(&io_ctx);
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.start_send(std::move(io));
  EXPECT_EQ(TestFixture::case_name, TestFixture::receive());

  io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  EXPECT_EQ(&io_ctx, io->template context<int>());
  EXPECT_EQ(nullptr, io->template context<socket_t>());
  EXPECT_EQ(&socket_ctx, io->template socket_context<int>());
  EXPECT_EQ(nullptr, io->template socket_context<socket_t>());

  auto result = io->template get_if<socket_t::send_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name.size(), result->transferred);
}


TYPED_TEST(net_async_stream_socket, start_send_not_connected) //{{{1
{
  auto io = TestFixture::queue.make_io();
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.start_send(std::move(io));

  io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::send_t>(error);
  ASSERT_NE(nullptr, result);
#if __sal_os_linux
  EXPECT_EQ(std::errc::broken_pipe, error);
#else
  EXPECT_EQ(std::errc::not_connected, error);
#endif
}


TYPED_TEST(net_async_stream_socket, start_send_before_shutdown) //{{{1
{
  TestFixture::connect();

  auto io = TestFixture::queue.make_io();
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.start_send(std::move(io));
  TestFixture::socket.shutdown(socket_t::shutdown_send);

  io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  auto result = io->template get_if<socket_t::send_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name.size(), result->transferred);
}


TYPED_TEST(net_async_stream_socket, start_send_after_shutdown) //{{{1
{
  TestFixture::connect();
  TestFixture::socket.shutdown(socket_t::shutdown_send);

  auto io = TestFixture::queue.make_io();
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.start_send(std::move(io));

  io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<socket_t::send_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::broken_pipe, error);
}


//}}}1


} // namespace
