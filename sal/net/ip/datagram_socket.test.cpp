#include <sal/net/ip/udp.hpp>
#include <sal/common.test.hpp>
#include <sal/buf_ptr.hpp>
#include <thread>


namespace {


using namespace std::chrono_literals;


struct datagram_socket
  : public sal_test::with_value<sal::net::ip::udp_t>
{
  using socket_t = sal::net::ip::udp_t::socket_t;

  const sal::net::ip::udp_t protocol = GetParam();

  const socket_t::endpoint_t endpoint =
    protocol == sal::net::ip::udp_t::v4
      ? socket_t::endpoint_t{sal::net::ip::address_v4_t::loopback, 8195}
      : socket_t::endpoint_t{sal::net::ip::address_v6_t::loopback, 8195}
  ;

  socket_t::endpoint_t remote_endpoint{endpoint.address(), 0};

  socket_t receiver{endpoint}, sender{remote_endpoint};
};


INSTANTIATE_TEST_CASE_P(net_ip, datagram_socket,
  ::testing::Values(
    sal::net::ip::udp_t::v4,
    sal::net::ip::udp_t::v6
  ),
);


TEST_P(datagram_socket, ctor)
{
  socket_t socket;
  EXPECT_FALSE(socket.is_open());
}


TEST_P(datagram_socket, ctor_move)
{
  EXPECT_TRUE(receiver.is_open());
  auto socket = std::move(receiver);
  EXPECT_TRUE(socket.is_open());
  EXPECT_FALSE(receiver.is_open());
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
  socket_t socket(protocol);
  EXPECT_TRUE(socket.is_open());
}


TEST_P(datagram_socket, ctor_handle)
{
  auto handle = sal::net::socket_base_t::invalid - 1;
  socket_t socket(handle);
  EXPECT_EQ(handle, socket.native_handle());

  std::error_code ignored;
  socket.close(ignored);
}


TEST_P(datagram_socket, ctor_endpoint)
{
  EXPECT_EQ(endpoint, receiver.local_endpoint());
}


TEST_P(datagram_socket, assign_move)
{
  EXPECT_TRUE(receiver.is_open());
  EXPECT_TRUE(sender.is_open());

  auto handle = sender.native_handle();
  receiver = std::move(sender);
  EXPECT_EQ(handle, receiver.native_handle());
  EXPECT_TRUE(receiver.is_open());
  EXPECT_FALSE(sender.is_open());
}


TEST_P(datagram_socket, receive_from_invalid)
{
  receiver.close();
  char buf[1024];

  {
    std::error_code error;
    EXPECT_EQ(0U, receiver.receive_from(buf, remote_endpoint, error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      (void)receiver.receive_from(buf, remote_endpoint),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, receive_from_no_sender_non_blocking)
{
  receiver.non_blocking(true);
  char buf[1024];

  {
    std::error_code error;
    EXPECT_EQ(0U, receiver.receive_from(buf, remote_endpoint, error));
    EXPECT_EQ(std::errc::operation_would_block, error);
  }

  {
    EXPECT_THROW(
      (void)receiver.receive_from(buf, remote_endpoint),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, send_to_invalid)
{
  sender.close();

  {
    std::error_code error;
    EXPECT_EQ(0U, sender.send_to(case_name, endpoint, error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      (void)sender.send_to(case_name, endpoint),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, send_to_and_receive_from)
{
  ASSERT_FALSE(receiver.wait(receiver.wait_read, 0s));

  EXPECT_EQ(case_name.size(), sender.send_to(case_name, endpoint));
  ASSERT_TRUE(receiver.wait(receiver.wait_read, 10s));

  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), receiver.receive_from(buf, remote_endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(sender.local_endpoint(), remote_endpoint);
}


TEST_P(datagram_socket, receive_from_less_than_send_to)
{
  ASSERT_FALSE(receiver.wait(receiver.wait_read, 0s));

  EXPECT_EQ(case_name.size(), sender.send_to(case_name, endpoint));
  ASSERT_TRUE(receiver.wait(receiver.wait_read, 10s));

  std::error_code error;
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(
    case_name.size() / 2,
    receiver.receive_from(
      sal::make_buf(buf, case_name.size() / 2),
      remote_endpoint,
      error
    )
  );
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(sender.local_endpoint(), remote_endpoint);
  EXPECT_FALSE(receiver.wait(receiver.wait_read, 0s));
}


TEST_P(datagram_socket, receive_from_peek)
{
  EXPECT_EQ(case_name.size(), sender.send_to(case_name, endpoint));
  char buf[1024];

  // peek
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(
    case_name.size(),
    receiver.receive_from(buf, remote_endpoint, receiver.peek)
  );
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(sender.local_endpoint(), remote_endpoint);

  // actually extract
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), receiver.receive_from(buf, remote_endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(sender.local_endpoint(), remote_endpoint);
}


TEST_P(datagram_socket, send_to_do_not_route)
{
  // sender
  EXPECT_EQ(
    case_name.size(),
    sender.send_to(case_name, endpoint, sender.do_not_route)
  );

  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));

  // receiver
  EXPECT_EQ(case_name.size(), receiver.receive_from(buf, remote_endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(sender.local_endpoint(), remote_endpoint);
}


TEST_P(datagram_socket, receive_invalid)
{
  receiver.close();
  char buf[1024];

  {
    std::error_code error;
    EXPECT_EQ(0U, receiver.receive(buf, error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      (void)receiver.receive(buf),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, receive_no_sender_non_blocking)
{
  receiver.non_blocking(true);
  char buf[1024];

  {
    std::error_code error;
    EXPECT_EQ(0U, receiver.receive(buf, error));
    EXPECT_EQ(std::errc::operation_would_block, error);
  }

  {
    EXPECT_THROW(
      (void)receiver.receive(buf),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, send_invalid)
{
  sender.close();

  {
    std::error_code error;
    EXPECT_EQ(0U, sender.send(case_name, error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      (void)sender.send(case_name),
      std::system_error
    );
  }
}


TEST_P(datagram_socket, send_not_connected)
{
  {
    std::error_code error;
    sender.send(case_name, error);
    EXPECT_EQ(std::errc::not_connected, error);
  }

  {
    EXPECT_THROW(sender.send(case_name), std::system_error);
  }
}


TEST_P(datagram_socket, send_and_receive)
{
  ASSERT_FALSE(receiver.wait(receiver.wait_read, 0s));

  ASSERT_NO_THROW(sender.connect(endpoint));
  EXPECT_EQ(case_name.size(), sender.send(case_name));

  ASSERT_TRUE(receiver.wait(receiver.wait_read, 10s));

  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), receiver.receive(buf));
  EXPECT_EQ(buf, case_name);
}


TEST_P(datagram_socket, receive_less_than_send)
{
  ASSERT_FALSE(receiver.wait(receiver.wait_read, 0s));

  ASSERT_NO_THROW(sender.connect(endpoint));
  EXPECT_EQ(case_name.size(), sender.send(case_name));

  ASSERT_TRUE(receiver.wait(receiver.wait_read, 10s));

  std::error_code error;
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(
    case_name.size() / 2,
    receiver.receive(sal::make_buf(buf, case_name.size() / 2), error)
  );
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_FALSE(receiver.wait(receiver.wait_read, 0s));
}


TEST_P(datagram_socket, receive_peek)
{
  // sender
  ASSERT_NO_THROW(sender.connect(endpoint));
  EXPECT_EQ(case_name.size(), sender.send(case_name));
  char buf[1024];

  // peek
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), receiver.receive(buf, receiver.peek));
  EXPECT_EQ(buf, case_name);

  // actually extract
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), receiver.receive(buf));
  EXPECT_EQ(buf, case_name);
}


TEST_P(datagram_socket, send_do_not_route)
{
  ASSERT_NO_THROW(sender.connect(endpoint));
  EXPECT_EQ(
    case_name.size(),
    sender.send(case_name, sender.do_not_route)
  );

  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), receiver.receive(buf));
  EXPECT_EQ(buf, case_name);
}


} // namespace
