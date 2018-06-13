#include <sal/net/ip/tcp.hpp>
#include <sal/common.test.hpp>
#include <sal/buf_ptr.hpp>
#include <thread>


namespace {


struct stream_socket
  : public sal_test::with_value<sal::net::ip::tcp_t>
{
  using socket_t = sal::net::ip::tcp_t::socket_t;
  using acceptor_t = sal::net::ip::tcp_t::acceptor_t;

  static constexpr sal::net::ip::port_t port = 8194;

  socket_t::endpoint_t loopback (const sal::net::ip::tcp_t &protocol) const
  {
    return protocol == sal::net::ip::tcp_t::v4()
      ? socket_t::endpoint_t(sal::net::ip::address_v4_t::loopback(), port)
      : socket_t::endpoint_t(sal::net::ip::address_v6_t::loopback(), port)
    ;
  }

  socket_t::endpoint_t other_loopback (const sal::net::ip::tcp_t &protocol) const
  {
    return protocol == sal::net::ip::tcp_t::v4()
      ? socket_t::endpoint_t(sal::net::ip::address_v6_t::loopback(), port)
      : socket_t::endpoint_t(sal::net::ip::address_v4_t::loopback(), port)
    ;
  }

  socket_t::endpoint_t any (const sal::net::ip::tcp_t &protocol) const
  {
    return protocol == sal::net::ip::tcp_t::v4()
      ? socket_t::endpoint_t(sal::net::ip::address_v4_t::any(), port)
      : socket_t::endpoint_t(sal::net::ip::address_v6_t::any(), port)
    ;
  }
};

constexpr sal::net::ip::port_t stream_socket::port;


INSTANTIATE_TEST_CASE_P(net_ip, stream_socket,
  ::testing::Values(
    sal::net::ip::tcp_t::v4(),
    sal::net::ip::tcp_t::v6()
  ),
);


using namespace std::chrono_literals;


TEST_P(stream_socket, ctor)
{
  socket_t socket;
  EXPECT_FALSE(socket.is_open());
}


TEST_P(stream_socket, ctor_move)
{
  socket_t a(GetParam());
  EXPECT_TRUE(a.is_open());
  auto b = std::move(a);
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TEST_P(stream_socket, ctor_move_no_handle)
{
  socket_t a;
  EXPECT_FALSE(a.is_open());
  auto b{std::move(a)};
  EXPECT_FALSE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TEST_P(stream_socket, ctor_protocol)
{
  socket_t socket(GetParam());
  EXPECT_TRUE(socket.is_open());
}


TEST_P(stream_socket, ctor_protocol_and_handle)
{
  auto handle = sal::net::socket_base_t::invalid - 1;
  socket_t socket(handle);
  EXPECT_EQ(handle, socket.native_handle());

  std::error_code ignored;
  socket.close(ignored);
}


TEST_P(stream_socket, ctor_endpoint)
{
  int family = GetParam().family() == AF_INET ? 0 : 1;

  static bool already_tested[2] = { false, false };
  if (already_tested[family])
  {
    // we can test this only once:
    // after bind() in ctor no use to set SO_REUSEADDR
    return;
  }
  already_tested[family] = true;

  socket_t::endpoint_t endpoint(GetParam(), 2 * port);
  socket_t socket(endpoint);

  endpoint = socket.local_endpoint();
  EXPECT_TRUE(endpoint.address().is_unspecified());
  EXPECT_EQ(2 * port, endpoint.port());
}


TEST_P(stream_socket, assign_move)
{
  socket_t a(GetParam()), b;
  EXPECT_TRUE(a.is_open());
  EXPECT_FALSE(b.is_open());

  auto handle = a.native_handle();
  b = std::move(a);
  EXPECT_EQ(handle, b.native_handle());
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TEST_P(stream_socket, receive_invalid)
{
  socket_t socket;

  char buf[1024];

  {
    std::error_code error;
    EXPECT_EQ(0U, socket.receive(buf, error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.receive(buf), std::system_error);
  }
}


TEST_P(stream_socket, send_invalid)
{
  socket_t socket;

  {
    std::error_code error;
    EXPECT_EQ(0U, socket.send(case_name, error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.send(case_name), std::system_error);
  }
}


TEST_P(stream_socket, send_not_connected)
{
  socket_t socket(GetParam());

  {
    std::error_code error;
    socket.send(case_name, error);
#if __sal_os_linux
    EXPECT_EQ(std::errc::broken_pipe, error);
#else
    EXPECT_EQ(std::errc::not_connected, error);
#endif
  }

  {
    EXPECT_THROW(socket.send(case_name), std::system_error);
  }
}


TEST_P(stream_socket, send_and_receive)
{
  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  EXPECT_EQ(case_name.size(), a.send(case_name));

  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));

  EXPECT_EQ(case_name.size(), b.receive(buf));
  EXPECT_EQ(case_name, buf);
}


TEST_P(stream_socket, receive_no_sender_non_blocking)
{
  acceptor_t acceptor(loopback(GetParam()));

  socket_t a;
  a.connect(loopback(GetParam()));

  auto b = acceptor.accept();
  b.non_blocking(true);

  char buf[1024];
  std::error_code error;
  EXPECT_EQ(0U, b.receive(buf, error));
  EXPECT_EQ(std::errc::operation_would_block, error);
}


TEST_P(stream_socket, receive_less_than_send)
{
  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  EXPECT_EQ(case_name.size(), a.send(case_name));

  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(
    case_name.size() / 2,
    b.receive(sal::make_buf(buf, case_name.size() / 2))
  );
  EXPECT_EQ(std::string(case_name, 0, case_name.size() / 2), buf);

  EXPECT_TRUE(b.wait(b.wait_read, 0s));
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size() - (case_name.size() / 2), b.receive(buf));
  EXPECT_EQ(std::string(case_name, case_name.size() / 2), buf);
}


TEST_P(stream_socket, receive_peek)
{
  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  EXPECT_EQ(case_name.size(), a.send(case_name));

  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), b.receive(buf, b.peek));
  EXPECT_EQ(case_name, buf);

  EXPECT_TRUE(b.wait(b.wait_read, 0s));
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), b.receive(buf));
  EXPECT_EQ(case_name, buf);
}


TEST_P(stream_socket, send_after_shutdown)
{
  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  a.shutdown(a.shutdown_send);

  {
    std::error_code error;
    a.send(case_name, error);
    EXPECT_EQ(std::errc::broken_pipe, error);
  }

  {
    EXPECT_THROW(a.send(case_name), std::system_error);
  }
}


TEST_P(stream_socket, send_after_remote_close)
{
  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  a.set_option(sal::net::linger(true, 0s));
  a.close();

  // give time RST to reach b
  std::this_thread::yield();

  {
    std::error_code error;
    b.send(case_name, error);
#if __sal_os_macos
    EXPECT_EQ(std::errc::broken_pipe, error);
#else
    EXPECT_EQ(std::errc::connection_reset, error);
#endif
  }

  {
    EXPECT_THROW(b.send(case_name), std::system_error);
  }
}


TEST_P(stream_socket, receive_after_shutdown)
{
  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));

  auto b = acceptor.accept();
  b.shutdown(b.shutdown_receive);

  char buf[1024];
  {
    std::error_code error;
    b.receive(buf, error);
    EXPECT_EQ(std::errc::broken_pipe, error);
  }

  {
    EXPECT_THROW(b.receive(buf), std::system_error);
  }
}


TEST_P(stream_socket, receive_after_remote_close)
{
  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  a.close();

  char buf[1024];

  {
    std::error_code error;
    b.receive(buf, error);
    EXPECT_EQ(std::errc::broken_pipe, error);
  }

  {
    EXPECT_THROW(b.receive(buf), std::system_error);
  }
}


TEST_P(stream_socket, send_do_not_route)
{
  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  EXPECT_EQ(case_name.size(), a.send(case_name, a.do_not_route));

  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));

  EXPECT_EQ(case_name.size(), b.receive(buf));
  EXPECT_EQ(case_name, buf);
}


TEST_P(stream_socket, no_delay)
{
  socket_t socket(GetParam());

  bool original, value;
  socket.get_option(socket_t::protocol_t::no_delay(&original));
  socket.set_option(socket_t::protocol_t::no_delay(!original));
  socket.get_option(socket_t::protocol_t::no_delay(&value));
  EXPECT_NE(original, value);
}


TEST_P(stream_socket, async_connect)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a(GetParam());
  a.associate(svc);

#if __sal_os_windows
  // ConnectEx requires socket to be bound
  auto endpoint = acceptor.local_endpoint();
  endpoint.port(endpoint.port() + 1);
  a.bind(endpoint);
#else
  a.non_blocking(true);
#endif

  a.async_connect(ctx.make_io(), acceptor.local_endpoint());
  auto b = acceptor.accept();

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = a.async_connect_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(&a, result->socket());

  EXPECT_EQ(nullptr, a.async_send_result(io));
}


TEST_P(stream_socket, async_connect_connection_refused)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  socket_t a(GetParam());
  a.associate(svc);

  auto endpoint = loopback(GetParam());
#if __sal_os_windows
  a.bind(endpoint);
#else
  a.non_blocking(true);
#endif

  endpoint.port(7);
  a.async_connect(ctx.make_io(), endpoint);

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = a.async_connect_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::connection_refused, error);

  EXPECT_THROW(
    a.async_connect_result(io),
    std::system_error
  );
}


TEST_P(stream_socket, async_connect_already_connected)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint = loopback(GetParam());
  acceptor_t acceptor(endpoint, true);

  socket_t a;
  a.connect(endpoint);
  a.associate(svc);
  auto b = acceptor.accept();

  endpoint.port(7);
  a.async_connect(ctx.make_io(), endpoint);

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = a.async_connect_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::already_connected, error);

  EXPECT_THROW(
    a.async_connect_result(io),
    std::system_error
  );
}


TEST_P(stream_socket, async_connect_address_not_available)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  socket_t a(GetParam());
  a.associate(svc);

#if __sal_os_windows
  a.bind(loopback(GetParam()));
#else
  a.non_blocking(true);
#endif

  a.async_connect(ctx.make_io(), any(GetParam()));

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = a.async_connect_result(io, error);
  ASSERT_NE(nullptr, result);

#if __sal_os_windows
  EXPECT_EQ(std::errc::address_not_available, error);
#else
  EXPECT_EQ(std::errc::connection_refused, error);
#endif

  EXPECT_THROW(
    a.async_connect_result(io),
    std::system_error
  );
}


TEST_P(stream_socket, async_connect_not_bound)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a(GetParam());
  a.associate(svc);
  a.async_connect(ctx.make_io(), acceptor.local_endpoint());

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = a.async_connect_result(io, error);
  ASSERT_NE(nullptr, result);

#if __sal_os_windows

  //
  // bound socket for async_connect is ConnectEx-specific requirement
  //

  EXPECT_EQ(std::errc::invalid_argument, error);

  EXPECT_THROW(
    a.async_connect_result(io),
    std::system_error
  );

#else

  EXPECT_TRUE(!error);

#endif
}


template <typename Op>
inline auto to_s (const sal::net::io_ptr &io, const Op *op)
{
  return std::string{
    static_cast<const char *>(io->data()),
    op->transferred()
  };
}


inline auto from_s (sal::net::async_service_t::context_t &ctx,
  const std::string &content) noexcept
{
  auto io = ctx.make_io();
  io->resize(content.size());
  std::memcpy(io->begin(), content.data(), content.size());
  return io;
}


TEST_P(stream_socket, async_receive)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  a.associate(svc);
  a.async_receive(ctx.make_io());
  EXPECT_EQ(case_name.size(), b.send(case_name));

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = a.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(&a, result->socket());
  EXPECT_EQ(case_name.size(), result->transferred());

  EXPECT_EQ(nullptr, a.async_send_result(io));
}


TEST_P(stream_socket, async_receive_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  a.associate(svc);
  EXPECT_EQ(case_name.size(), b.send(case_name));
  a.async_receive(ctx.make_io());

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = a.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());

  EXPECT_EQ(nullptr, a.async_send_result(io));
}


TEST_P(stream_socket, async_receive_two_send)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();
  a.associate(svc);

  EXPECT_EQ(case_name.size(), b.send(case_name));
  EXPECT_EQ(case_name.size(), b.send(case_name));
  a.async_receive(ctx.make_io());

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = a.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(2 * case_name.size(), result->transferred());
  EXPECT_EQ(case_name + case_name, to_s(io, result));

  EXPECT_EQ(nullptr, a.async_send_result(io));
}


TEST_P(stream_socket, async_receive_two_send_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();
  a.associate(svc);

  a.async_receive(ctx.make_io());
  a.async_receive(ctx.make_io());
  EXPECT_EQ(case_name.size(), b.send(case_name));
  EXPECT_EQ(case_name.size(), b.send(case_name));

  for (int i = 0;  i < 2;  ++i)
  {
    auto io = ctx.poll();
    ASSERT_NE(nullptr, io);

    auto result = a.async_receive_result(io);
    ASSERT_NE(nullptr, result);

    if (case_name.size() == result->transferred())
    {
      EXPECT_EQ(case_name, to_s(io, result));
    }
    else
    {
      EXPECT_EQ(case_name + case_name, to_s(io, result));
      break;
    }
  }
}


TEST_P(stream_socket, async_receive_less_than_send)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();
  a.associate(svc);

  auto io = ctx.make_io();
  io->resize(case_name.size() / 2);
  a.async_receive(std::move(io));

  io = ctx.make_io();
  io->resize(case_name.size() - case_name.size() / 2);
  a.async_receive(std::move(io));

  EXPECT_EQ(case_name.size(), b.send(case_name));

  std::string data;
  for (auto i = 0;  i < 2;  ++i)
  {
    io = ctx.poll();
    ASSERT_NE(nullptr, io);
    auto result = a.async_receive_result(io);
    ASSERT_NE(nullptr, result);
    data += to_s(io, result);
  }
  EXPECT_EQ(case_name, data);
}


TEST_P(stream_socket, async_receive_less_than_send_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();
  a.associate(svc);

  EXPECT_EQ(case_name.size(), b.send(case_name));

  auto io = ctx.make_io();
  io->resize(case_name.size() / 2);
  a.async_receive(std::move(io));

  io = ctx.make_io();
  io->resize(case_name.size() - case_name.size() / 2);
  a.async_receive(std::move(io));

  std::string data;
  for (auto i = 0;  i < 2;  ++i)
  {
    io = ctx.poll();
    ASSERT_NE(nullptr, io);
    auto result = a.async_receive_result(io);
    ASSERT_NE(nullptr, result);
    data += to_s(io, result);
  }
  EXPECT_EQ(case_name, data);
}


TEST_P(stream_socket, async_receive_disconnected)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  a.associate(svc);
  auto b = acceptor.accept();

  a.async_receive(ctx.make_io());
  b.close();

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = a.async_receive_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred());
  EXPECT_EQ(std::errc::broken_pipe, error);
}


TEST_P(stream_socket, async_receive_disconnected_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  a.associate(svc);
  auto b = acceptor.accept();

  b.close();
  std::this_thread::yield();

  a.async_receive(ctx.make_io());

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = a.async_receive_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred());
  EXPECT_EQ(std::errc::broken_pipe, error);
}


TEST_P(stream_socket, async_receive_peek)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  a.associate(svc);
  a.async_receive(ctx.make_io(), a.peek);
  EXPECT_EQ(case_name.size(), b.send(case_name));

  // receive with peek
  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);
  auto result = a.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));

  // receive that actually consumes
  a.async_receive(ctx.make_io());
  io = ctx.poll();
  ASSERT_NE(nullptr, io);
  result = a.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));
}


TEST_P(stream_socket, async_receive_peek_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  a.associate(svc);
  EXPECT_EQ(case_name.size(), b.send(case_name));
  a.async_receive(ctx.make_io(), a.peek);

  // receive with peek
  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);
  auto result = a.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));

  // receive that actually consumes
  a.async_receive(ctx.make_io());
  io = ctx.poll();
  ASSERT_NE(nullptr, io);
  result = a.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));
}


TEST_P(stream_socket, async_receive_before_shutdown)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();
  a.associate(svc);

  a.async_receive(ctx.make_io());
  a.shutdown(a.shutdown_receive);

  EXPECT_EQ(case_name.size(), b.send(case_name));

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

#if __sal_os_macos

  std::error_code error;
  auto result = a.async_receive_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::broken_pipe, error);

  EXPECT_THROW(
    a.async_receive_result(io),
    std::system_error
  );

#else

  auto result = a.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());

#endif
}


TEST_P(stream_socket, async_receive_after_shutdown)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();
  a.associate(svc);

  a.shutdown(a.shutdown_receive);
  a.async_receive(ctx.make_io());

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = a.async_receive_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::broken_pipe, error);

  EXPECT_THROW(
    a.async_receive_result(io),
    std::system_error
  );
}


TEST_P(stream_socket, async_send)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  a.associate(svc);
  a.async_send(from_s(ctx, case_name));

  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), b.receive(buf));
  EXPECT_EQ(case_name, buf);

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = a.async_send_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(&a, result->socket());
  EXPECT_EQ(case_name, to_s(io, result));

  EXPECT_EQ(nullptr, a.async_receive_result(io));
}


TEST_P(stream_socket, async_send_not_connected)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  socket_t a(GetParam());
  a.associate(svc);

  a.async_send(from_s(ctx, case_name));

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = a.async_send_result(io, error);
  ASSERT_NE(nullptr, result);

#if __sal_os_linux
  EXPECT_EQ(std::errc::broken_pipe, error);
#else
  EXPECT_EQ(std::errc::not_connected, error);
#endif
}


TEST_P(stream_socket, async_send_before_shutdown)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  a.associate(svc);
  a.async_send(from_s(ctx, case_name));
  a.shutdown(a.shutdown_send);

  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), b.receive(buf));
  EXPECT_EQ(case_name, buf);

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = a.async_send_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));
}


TEST_P(stream_socket, async_send_after_shutdown)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  a.associate(svc);
  a.shutdown(a.shutdown_send);
  a.async_send(from_s(ctx, case_name));

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = a.async_send_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::broken_pipe, error);

  EXPECT_THROW(
    a.async_send_result(io),
    std::system_error
  );
}


TEST_P(stream_socket, async_send_no_receive)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  auto b = acceptor.accept();

  a.associate(svc);
  a.async_send(from_s(ctx, case_name));

  // async send success only tells that OS sent buffer, not whether it
  // actually reached there
  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = a.async_send_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));
}


TEST_P(stream_socket, async_send_disconnected)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  a.associate(svc);
  auto b = acceptor.accept();

  a.async_send(from_s(ctx, case_name));
  b.close();

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = a.async_send_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));
}


TEST_P(stream_socket, async_send_disconnected_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));
  a.associate(svc);
  auto b = acceptor.accept();

  b.close();
  std::this_thread::yield();
  a.async_send(from_s(ctx, case_name));

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = a.async_send_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));
}


} // namespace
