#include <sal/net/ip/udp.hpp>
#include <sal/net/io_service.hpp>
#include <sal/net/io_context.hpp>
#include <sal/common.test.hpp>


namespace {


using namespace std::chrono_literals;


struct datagram_socket
  : public sal_test::with_value<sal::net::ip::udp_t>
{
  using socket_t = sal::net::ip::udp_t::socket_t;
  static constexpr sal::net::ip::port_t port = 8192;

  socket_t::endpoint_t loopback (const sal::net::ip::udp_t &protocol) const
  {
    return protocol == sal::net::ip::udp_t::v4()
      ? socket_t::endpoint_t(sal::net::ip::address_v4_t::loopback(), port)
      : socket_t::endpoint_t(sal::net::ip::address_v6_t::loopback(), port)
    ;
  }

  static auto &service ()
  {
    static sal::net::io_service_t svc;
    return svc;
  }

  static auto &context ()
  {
    static sal::net::io_context_t ctx = service().make_context();
    return ctx;
  }

  auto make_buf ()
  {
    return context().make_buf();
  }
};

constexpr sal::net::ip::port_t datagram_socket::port;


INSTANTIATE_TEST_CASE_P(net_ip, datagram_socket,
  ::testing::Values(
    sal::net::ip::udp_t::v4(),
    sal::net::ip::udp_t::v6()
  )
);


TEST_P(datagram_socket, ctor)
{
  socket_t socket;
  EXPECT_FALSE(socket.is_open());
}


TEST_P(datagram_socket, ctor_move)
{
  socket_t a(GetParam());
  EXPECT_TRUE(a.is_open());
  auto b = std::move(a);
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TEST_P(datagram_socket, ctor_move_no_handle)
{
  socket_t a;
  EXPECT_FALSE(a.is_open());
  auto b{std::move(a)};
  EXPECT_FALSE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TEST_P(datagram_socket, ctor_protocol)
{
  socket_t socket(GetParam());
  EXPECT_TRUE(socket.is_open());
}


TEST_P(datagram_socket, ctor_protocol_and_handle)
{
  auto handle = sal::net::socket_base_t::invalid_socket - 1;
  socket_t socket(handle);
  EXPECT_EQ(handle, socket.native_handle());

  std::error_code ignored;
  socket.close(ignored);
}


TEST_P(datagram_socket, ctor_endpoint)
{
  socket_t::endpoint_t endpoint(GetParam(), port);
  socket_t socket(endpoint);

  endpoint = socket.local_endpoint();
  EXPECT_TRUE(endpoint.address().is_unspecified());
  EXPECT_EQ(port, endpoint.port());
}


TEST_P(datagram_socket, assign_move)
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


TEST_P(datagram_socket, receive_from_invalid)
{
  socket_t::endpoint_t endpoint;
  socket_t socket;

  char buf[1024];

  {
    std::error_code error;
    EXPECT_EQ(0U, socket.receive_from(sal::make_buf(buf), endpoint, error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      (void)socket.receive_from(sal::make_buf(buf), endpoint),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, receive_from_no_sender_non_blocking)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.non_blocking(true);

  char buf[1024];

  {
    std::error_code error;
    EXPECT_EQ(0U, socket.receive_from(sal::make_buf(buf), endpoint, error));
    EXPECT_EQ(std::errc::operation_would_block, error);
  }

  {
    EXPECT_THROW(
      (void)socket.receive_from(sal::make_buf(buf), endpoint),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, send_to_invalid)
{
  socket_t::endpoint_t endpoint;
  socket_t socket;

  {
    std::error_code error;
    EXPECT_EQ(0U, socket.send_to(sal::make_buf(case_name), endpoint, error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      (void)socket.send_to(sal::make_buf(case_name), endpoint),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, send_to_and_receive_from)
{
  socket_t::endpoint_t ra(loopback(GetParam())), sa(ra.address(), ra.port() + 1);
  socket_t r(ra), s(sa);

  ASSERT_FALSE(r.wait(r.wait_read, 0s));

  // sender
  {
    EXPECT_EQ(case_name.size(), s.send_to(sal::make_buf(case_name), ra));
  }

  ASSERT_TRUE(r.wait(r.wait_read, 10s));

  // receiver
  {
    socket_t::endpoint_t endpoint;
    char buf[1024];
    std::memset(buf, '\0', sizeof(buf));
    EXPECT_EQ(case_name.size(), r.receive_from(sal::make_buf(buf), endpoint));
    EXPECT_EQ(buf, case_name);
    EXPECT_EQ(sa, endpoint);
  }
}


TEST_P(datagram_socket, receive_from_less_than_send_to)
{
  socket_t::endpoint_t ra(loopback(GetParam())), sa(ra.address(), ra.port() + 1);
  socket_t r(ra), s(sa);

  ASSERT_FALSE(r.wait(r.wait_read, 0s));

  // sender
  {
    EXPECT_EQ(case_name.size(), s.send_to(sal::make_buf(case_name), ra));
  }

  ASSERT_TRUE(r.wait(r.wait_read, 10s));

  // receiver
  {
    std::error_code error;
    socket_t::endpoint_t endpoint;
    char buf[1024];
    std::memset(buf, '\0', sizeof(buf));
    EXPECT_EQ(0U,
      r.receive_from(sal::make_buf(buf, case_name.size() / 2), endpoint, error)
    );
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_FALSE(r.wait(r.wait_read, 0s));
  }
}


TEST_P(datagram_socket, receive_from_peek)
{
  socket_t::endpoint_t ra(loopback(GetParam())), sa(ra.address(), ra.port() + 1);
  socket_t r(ra), s(sa);

  // sender
  EXPECT_EQ(case_name.size(), s.send_to(sal::make_buf(case_name), ra));

  socket_t::endpoint_t endpoint;
  char buf[1024];

  // receiver: peek
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), r.receive_from(sal::make_buf(buf), endpoint, r.peek));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(sa, endpoint);

  // receiver: actually extract
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), r.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(sa, endpoint);
}


TEST_P(datagram_socket, send_to_do_not_route)
{
  socket_t::endpoint_t ra(loopback(GetParam())), sa(ra.address(), ra.port() + 1);
  socket_t r(ra), s(sa);

  // sender
  EXPECT_EQ(
    case_name.size(),
    s.send_to(sal::make_buf(case_name), ra, s.do_not_route)
  );

  socket_t::endpoint_t endpoint;
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));

  // receiver
  EXPECT_EQ(case_name.size(), r.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(sa, endpoint);
}


TEST_P(datagram_socket, receive_invalid)
{
  socket_t socket;

  char buf[1024];

  {
    std::error_code error;
    EXPECT_EQ(0U, socket.receive(sal::make_buf(buf), error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      (void)socket.receive(sal::make_buf(buf)),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, receive_no_sender_non_blocking)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.non_blocking(true);

  char buf[1024];

  {
    std::error_code error;
    EXPECT_EQ(0U, socket.receive(sal::make_buf(buf), error));
    EXPECT_EQ(std::errc::operation_would_block, error);
  }

  {
    EXPECT_THROW(
      (void)socket.receive(sal::make_buf(buf)),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, send_invalid)
{
  socket_t socket;

  {
    std::error_code error;
    EXPECT_EQ(0U, socket.send(sal::make_buf(case_name), error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      (void)socket.send(sal::make_buf(case_name)),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, send_not_connected)
{
  socket_t socket(loopback(GetParam()));

  {
    std::error_code error;
    socket.send(sal::make_buf(case_name), error);
    EXPECT_EQ(std::errc::destination_address_required, error);
  }

  {
    EXPECT_THROW(socket.send(sal::make_buf(case_name)), std::system_error);
  }
}


TEST_P(datagram_socket, send_and_receive)
{
  socket_t::endpoint_t ra(loopback(GetParam()));
  socket_t r(ra), s(GetParam());

  ASSERT_FALSE(r.wait(r.wait_read, 0s));

  // sender
  {
    ASSERT_NO_THROW(s.connect(ra));
    EXPECT_EQ(case_name.size(), s.send(sal::make_buf(case_name)));
  }

  ASSERT_TRUE(r.wait(r.wait_read, 10s));

  // receiver
  {
    char buf[1024];
    std::memset(buf, '\0', sizeof(buf));
    EXPECT_EQ(case_name.size(), r.receive(sal::make_buf(buf)));
    EXPECT_EQ(buf, case_name);
  }
}


TEST_P(datagram_socket, receive_less_than_send)
{
  socket_t::endpoint_t ra(loopback(GetParam()));
  socket_t r(ra), s(GetParam());

  ASSERT_FALSE(r.wait(r.wait_read, 0s));

  // sender
  {
    ASSERT_NO_THROW(s.connect(ra));
    EXPECT_EQ(case_name.size(), s.send(sal::make_buf(case_name)));
  }

  ASSERT_TRUE(r.wait(r.wait_read, 10s));

  // receiver
  {
    std::error_code error;
    char buf[1024];
    std::memset(buf, '\0', sizeof(buf));
    EXPECT_EQ(0U, r.receive(sal::make_buf(buf, case_name.size() / 2), error));
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_FALSE(r.wait(r.wait_read, 0s));
  }
}


TEST_P(datagram_socket, receive_peek)
{
  socket_t::endpoint_t ra(loopback(GetParam()));
  socket_t r(ra), s(GetParam());

  // sender
  ASSERT_NO_THROW(s.connect(ra));
  EXPECT_EQ(case_name.size(), s.send(sal::make_buf(case_name)));

  char buf[1024];

  // receiver: peek
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), r.receive(sal::make_buf(buf), r.peek));
  EXPECT_EQ(buf, case_name);

  // receiver: actually extract
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), r.receive(sal::make_buf(buf)));
  EXPECT_EQ(buf, case_name);
}


TEST_P(datagram_socket, send_do_not_route)
{
  socket_t::endpoint_t ra(loopback(GetParam()));
  socket_t r(ra), s(GetParam());

  // sender
  ASSERT_NO_THROW(s.connect(ra));
  EXPECT_EQ(
    case_name.size(),
    s.send(sal::make_buf(case_name), s.do_not_route)
  );

  socket_t::endpoint_t endpoint;
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));

  // receiver
  EXPECT_EQ(case_name.size(), r.receive(sal::make_buf(buf)));
  EXPECT_EQ(buf, case_name);
}


TEST_P(datagram_socket, async_receive_from)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));

  socket_t socket(endpoint);
  service().associate(socket);

  auto io_buf = make_buf();
  io_buf->user_data(1);
  socket.async_receive_from(std::move(io_buf));

  socket.send_to(sal::make_buf(case_name), endpoint);

  io_buf = context().get();
  ASSERT_NE(nullptr, io_buf);
  EXPECT_EQ(1U, io_buf->user_data());

  auto result = socket.async_receive_from_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(endpoint, result->endpoint());
  EXPECT_EQ(0, result->flags());
  EXPECT_EQ(case_name, std::string(io_buf->data(), io_buf->size()));
}


TEST_P(datagram_socket, async_receive_from_immediate_completion)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));

  socket_t socket(endpoint);
  service().associate(socket);

  socket.send_to(sal::make_buf(case_name), endpoint);

  auto io_buf = make_buf();
  io_buf->user_data(2);
  socket.async_receive_from(std::move(io_buf));

  io_buf = context().get();
  ASSERT_NE(nullptr, io_buf);
  EXPECT_EQ(2U, io_buf->user_data());

  auto result = socket.async_receive_from_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(endpoint, result->endpoint());
  EXPECT_EQ(0, result->flags());
  EXPECT_EQ(case_name, std::string(io_buf->data(), io_buf->size()));
}


TEST_P(datagram_socket, async_receive_from_no_data)
{
  {
    socket_t::endpoint_t endpoint(loopback(GetParam()));

    socket_t socket(endpoint);
    service().associate(socket);

    socket.async_receive_from(make_buf());

    EXPECT_FALSE(context().wait(10ms));
    EXPECT_EQ(nullptr, context().try_get());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_from_result(context().get()),
    std::system_error
  );
}


} // namespace
