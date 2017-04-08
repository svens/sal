#include <sal/net/ip/udp.hpp>
#include <sal/net/io_context.hpp>
#include <sal/net/io_service.hpp>
#include <sal/common.test.hpp>
#include <thread>


#include <sal/thread.hpp>


namespace {


using namespace std::chrono_literals;


struct datagram_socket
  : public sal_test::with_value<sal::net::ip::udp_t>
{
  using socket_t = sal::net::ip::udp_t::socket_t;
  static constexpr sal::net::ip::port_t port = 8195;

  socket_t::endpoint_t loopback (const sal::net::ip::udp_t &protocol) const
  {
    return protocol == sal::net::ip::udp_t::v4()
      ? socket_t::endpoint_t(sal::net::ip::address_v4_t::loopback(), port)
      : socket_t::endpoint_t(sal::net::ip::address_v6_t::loopback(), port)
    ;
  }

  sal::net::io_service_t service;
  sal::net::io_context_t context = service.make_context();

  sal::net::io_buf_ptr make_buf (const std::string &content) noexcept
  {
    auto io_buf = context.make_buf();
    io_buf->resize(content.size());
    std::memcpy(io_buf->data(), content.data(), content.size());
    return io_buf;
  }

  static std::string to_string (const sal::net::io_buf_ptr &io_buf, size_t size)
  {
    return std::string(static_cast<const char *>(io_buf->data()), size);
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
  auto endpoint = loopback(GetParam());
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
  socket_t socket(loopback(GetParam()));
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
    EXPECT_EQ(std::errc::not_connected, error);
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
  service.associate(socket);

  auto io_buf = context.make_buf();
  io_buf->user_data(1);
  socket.async_receive_from(std::move(io_buf));

  socket.send_to(sal::make_buf(case_name), endpoint);

  io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  EXPECT_EQ(1U, io_buf->user_data());

  auto result = socket.async_receive_from_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(endpoint, result->endpoint());
  EXPECT_EQ(case_name, to_string(io_buf, result->transferred()));

  // TODO(restore) EXPECT_EQ(nullptr, socket.async_send_to_result(io_buf));
}


TEST_P(datagram_socket, async_receive_from_immediate_completion)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  socket.send_to(sal::make_buf(case_name), endpoint);

  auto io_buf = context.make_buf();
  io_buf->user_data(2);
  socket.async_receive_from(std::move(io_buf));

  io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  EXPECT_EQ(2U, io_buf->user_data());

  auto result = socket.async_receive_from_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(endpoint, result->endpoint());
  EXPECT_EQ(case_name, to_string(io_buf, result->transferred()));
}


TEST_P(datagram_socket, async_receive_from_partially_immediate_completion)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  // start three writes and one read
  socket.async_receive_from(context.make_buf());
  socket.send_to(sal::make_buf(case_name + "_one"), endpoint);
  socket.send_to(sal::make_buf(case_name + "_two"), endpoint);
  socket.send_to(sal::make_buf(case_name + "_three"), endpoint);

  // first read must succeed
  // (and in case of Reactor, fetch poller event)
  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  auto result = socket.async_receive_from_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name + "_one", to_string(io_buf, result->transferred()));

  // launch thread that will "steal" packet 'two'
  auto t = std::thread([&]
  {
    auto context1 = service.make_context();
    socket.async_receive_from(context1.make_buf());
    auto io_buf = context1.get();
    ASSERT_NE(nullptr, io_buf);
    auto result = socket.async_receive_from_result(io_buf);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(case_name + "_two", to_string(io_buf, result->transferred()));
    EXPECT_EQ(&context1, &io_buf->this_context());
  });
  t.join();

  // start remaining of reads (one will fail because of "stolen" packet)
  socket.async_receive_from(context.make_buf());
  socket.async_receive_from(context.make_buf());

  // second this thread's reads third and final packet
  io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  result = socket.async_receive_from_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name + "_three", to_string(io_buf, result->transferred()));

  // there is pending read but no data anymore
  io_buf = context.get(1ms);
  EXPECT_EQ(nullptr, io_buf);

  // close socket
  socket.close();
  io_buf = context.get(1ms);
  ASSERT_NE(nullptr, io_buf);

  // closing will cancel read
  std::error_code error;
  result = socket.async_receive_from_result(io_buf, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred());
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TEST_P(datagram_socket, async_receive_from_invalid)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  socket.async_receive_from(context.make_buf());
  socket.close();

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  std::error_code error;
  auto result = socket.async_receive_from_result(io_buf, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
  EXPECT_EQ(0U, result->transferred());

  EXPECT_THROW(
    socket.async_receive_from_result(io_buf),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_from_invalid_immediate_completion)
{
  socket_t socket(loopback(GetParam()));
  service.associate(socket);

  socket.close();
  socket.async_receive_from(context.make_buf());

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  std::error_code error;
  auto result = socket.async_receive_from_result(io_buf, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::bad_file_descriptor, error);
  EXPECT_EQ(0U, result->transferred());

  EXPECT_THROW(
    socket.async_receive_from_result(io_buf),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_from_no_sender)
{
  {
    socket_t socket(loopback(GetParam()));
    service.associate(socket);

    socket.async_receive_from(context.make_buf());

    EXPECT_EQ(nullptr, context.get(0s));
    EXPECT_EQ(nullptr, context.try_get());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_from_result(context.get()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_from_peek)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  socket.async_receive_from(context.make_buf(), socket.peek);

  socket.send_to(sal::make_buf(case_name), endpoint);

  // regardless of peek, completion should be removed from queue
  ASSERT_NE(nullptr, context.get());
  EXPECT_EQ(nullptr, context.get(0s));
}


TEST_P(datagram_socket, async_receive_from_peek_immediate_completion)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  socket.send_to(sal::make_buf(case_name), endpoint);

  socket.async_receive_from(context.make_buf(), socket.peek);

  // regardless of peek, completion should be removed from queue
  ASSERT_NE(nullptr, context.get());
  EXPECT_EQ(nullptr, context.get(0s));
}


TEST_P(datagram_socket, async_receive_from_less_than_send)
{
  {
    socket_t::endpoint_t endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    service.associate(socket);

    auto io_buf = context.make_buf();
    io_buf->resize(case_name.size() / 2);
    socket.async_receive_from(std::move(io_buf));

    socket.send_to(sal::make_buf(case_name), endpoint);

    io_buf = context.get();
    ASSERT_NE(nullptr, io_buf);

    std::error_code error;
    auto result = socket_t::async_receive_from_result(io_buf, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with partial 1st read, 2nd should have nothing
    io_buf->reset();
    socket.async_receive_from(std::move(io_buf));
    EXPECT_EQ(nullptr, context.get(0s));
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_from_result(context.get()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_from_less_than_send_immediate_completion)
{
  {
    socket_t::endpoint_t endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    service.associate(socket);

    socket.send_to(sal::make_buf(case_name), endpoint);

    auto io_buf = context.make_buf();
    io_buf->resize(case_name.size() / 2);
    socket.async_receive_from(std::move(io_buf));

    io_buf = context.get();
    ASSERT_NE(nullptr, io_buf);

    std::error_code error;
    auto result = socket_t::async_receive_from_result(io_buf, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with partial 1st read, 2nd should have nothing
    io_buf->reset();
    socket.async_receive_from(std::move(io_buf));
    EXPECT_EQ(nullptr, context.get(0s));
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_from_result(context.get()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_from_empty_buf)
{
  // couldn't unify IOCP/epoll/kqueue behaviour without additional syscall
  // but this is weird case anyway, prefer performance over unification

  {
    socket_t::endpoint_t endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    service.associate(socket);

    auto io_buf = context.make_buf();
    io_buf->resize(0);
    socket.async_receive_from(std::move(io_buf));

    socket.send_to(sal::make_buf(case_name), endpoint);

    io_buf = context.get();
    ASSERT_NE(nullptr, io_buf);

    std::error_code error;
    auto result = socket_t::async_receive_from_result(io_buf, error);
    ASSERT_NE(nullptr, result);

#if __sal_os_linux || __sal_os_windows

    // 1st receive is empty (because buf is empty)
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // buf 2nd receive still have nothing
    io_buf->reset();
    socket.async_receive_from(std::move(io_buf));
    EXPECT_EQ(nullptr, context.get(0s));

#elif __sal_os_darwin

    // 1st succeed immediately with 0B transferred
    EXPECT_TRUE(!error);
    EXPECT_EQ(0U, result->transferred());

    io_buf->reset();
    socket.async_receive_from(std::move(io_buf));

    // 2nd succeeds with originally sent packet
    io_buf = context.get();
    ASSERT_NE(nullptr, io_buf);
    result = socket_t::async_receive_from_result(io_buf, error);
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(!error);
    EXPECT_EQ(case_name, to_string(io_buf, result->transferred()));

#endif
  }

#if __sal_os_linux || __sal_os_windows
  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_from_result(context.get()),
    std::system_error
  );
#endif
}


TEST_P(datagram_socket, async_receive_from_empty_buf_immediate_completion)
{
  {
    socket_t::endpoint_t endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    service.associate(socket);

    socket.send_to(sal::make_buf(case_name), endpoint);
    std::this_thread::sleep_for(1ms);

    auto io_buf = context.make_buf();
    io_buf->resize(0);
    socket.async_receive_from(std::move(io_buf));

    io_buf = context.get();
    ASSERT_NE(nullptr, io_buf);

    std::error_code error;
    auto result = socket_t::async_receive_from_result(io_buf, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with empty 1st read, 2nd should have nothing
    io_buf->reset();
    socket.async_receive_from(std::move(io_buf));
    EXPECT_EQ(nullptr, context.get(0s));
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_from_result(context.get()),
    std::system_error
  );
}


#if !__sal_os_linux


TEST_P(datagram_socket, async_receive)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  socket.async_receive(context.make_buf());
  socket.send_to(sal::make_buf(case_name), endpoint);

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  auto result = socket.async_receive_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_string(io_buf, result->transferred()));

  EXPECT_EQ(nullptr, socket.async_send_result(io_buf));
}


TEST_P(datagram_socket, async_receive_immediate_completion)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  socket.send_to(sal::make_buf(case_name), endpoint);
  socket.async_receive(context.make_buf());

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  auto result = socket.async_receive_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_string(io_buf, result->transferred()));
}


TEST_P(datagram_socket, async_receive_connected)
{
  socket_t socket(loopback(GetParam()));
  socket.connect(socket.local_endpoint());
  service.associate(socket);

  socket.async_receive(context.make_buf());
  socket.send(sal::make_buf(case_name));

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  auto result = socket.async_receive_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_string(io_buf, result->transferred()));
}


TEST_P(datagram_socket, async_receive_connected_immediate_completion)
{
  socket_t socket(loopback(GetParam()));
  socket.connect(socket.local_endpoint());
  service.associate(socket);

  socket.send(sal::make_buf(case_name));
  socket.async_receive(context.make_buf());

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  auto result = socket.async_receive_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_string(io_buf, result->transferred()));
}


TEST_P(datagram_socket, async_receive_connected_elsewhere)
{
  {
    socket_t receiver(GetParam()), sender(GetParam());
    service.associate(receiver);

    receiver.connect(loopback(GetParam()));
    receiver.async_receive(context.make_buf());
    sender.send_to(sal::make_buf(case_name), receiver.local_endpoint());

    // must be ignored if from elsewehere than connected
    EXPECT_EQ(nullptr, context.get(0s));
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(context.get()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_connected_elsewhere_immediate_completion)
{
  {
    socket_t receiver(GetParam()), sender(GetParam());
    service.associate(receiver);

    receiver.connect(loopback(GetParam()));
    sender.send_to(sal::make_buf(case_name), receiver.local_endpoint());
    receiver.async_receive(context.make_buf());

    // must be ignored if from elsewehere than connected
    EXPECT_EQ(nullptr, context.get(0s));
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(context.get()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_invalid)
{
  socket_t socket(loopback(GetParam()));
  service.associate(socket);

  socket.async_receive(context.make_buf());
  socket.close();

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  std::error_code error;
  auto result = socket.async_receive_result(io_buf, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
  EXPECT_EQ(0U, result->transferred());

  EXPECT_THROW(socket.async_receive_result(io_buf), std::system_error);
}


TEST_P(datagram_socket, async_receive_invalid_immediate_completion)
{
  socket_t socket(loopback(GetParam()));
  service.associate(socket);

  socket.close();
  socket.async_receive(context.make_buf());

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  std::error_code error;
  auto result = socket.async_receive_result(io_buf, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::bad_file_descriptor, error);
  EXPECT_EQ(0U, result->transferred());

  EXPECT_THROW(socket.async_receive_result(io_buf), std::system_error);
}


TEST_P(datagram_socket, async_receive_no_sender)
{
  {
    socket_t socket(loopback(GetParam()));
    service.associate(socket);

    socket.async_receive(context.make_buf());

    EXPECT_EQ(nullptr, context.get(0s));
    EXPECT_EQ(nullptr, context.try_get());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(context.get()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_peek)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  socket.async_receive(context.make_buf(), socket.peek);
  socket.send_to(sal::make_buf(case_name), endpoint);

  // regardless of peek, completion should be removed from queue
  ASSERT_NE(nullptr, context.get());
  EXPECT_EQ(nullptr, context.get(0s));
}


TEST_P(datagram_socket, async_receive_peek_immediate_completion)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  socket.send_to(sal::make_buf(case_name), endpoint);
  socket.async_receive(context.make_buf(), socket.peek);

  // regardless of peek, completion should be removed from queue
  ASSERT_NE(nullptr, context.get());
  EXPECT_EQ(nullptr, context.get(0s));
}


TEST_P(datagram_socket, async_receive_less_than_send)
{
  {
    socket_t::endpoint_t endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    service.associate(socket);

    auto io_buf = context.make_buf();
    io_buf->resize(case_name.size() / 2);
    socket.async_receive(std::move(io_buf));

    socket.send_to(sal::make_buf(case_name), endpoint);

    io_buf = context.get();
    ASSERT_NE(nullptr, io_buf);

    std::error_code error;
    auto result = socket_t::async_receive_result(io_buf, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with partial 1st read, 2nd should have nothing
    io_buf->reset();
    socket.async_receive(std::move(io_buf));
    EXPECT_EQ(nullptr, context.get(0s));
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(context.get()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_less_than_send_immediate_completion)
{
  {
    socket_t::endpoint_t endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    service.associate(socket);

    socket.send_to(sal::make_buf(case_name), endpoint);

    auto io_buf = context.make_buf();
    io_buf->resize(case_name.size() / 2);
    socket.async_receive(std::move(io_buf));

    io_buf = context.get();
    ASSERT_NE(nullptr, io_buf);

    std::error_code error;
    auto result = socket_t::async_receive_result(io_buf, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with partial 1st read, 2nd should have nothing
    io_buf->reset();
    socket.async_receive(std::move(io_buf));
    EXPECT_EQ(nullptr, context.get(0s));
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(context.get()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_empty_buf)
{
  // couldn't unify IOCP/kqueue behaviour without additional syscall
  // but this is weird case anyway, prefer performance over unification

  {
    socket_t::endpoint_t endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    service.associate(socket);

    auto io_buf = context.make_buf();
    io_buf->resize(0);
    socket.async_receive(std::move(io_buf));

    socket.send_to(sal::make_buf(case_name), endpoint);

    io_buf = context.get();
    ASSERT_NE(nullptr, io_buf);

    std::error_code error;
    auto result = socket_t::async_receive_result(io_buf, error);
    ASSERT_NE(nullptr, result);

#if __sal_os_windows

    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with empty 1st read, 2nd should have nothing
    io_buf->reset();
    socket.async_receive(std::move(io_buf));
    EXPECT_EQ(nullptr, context.get(0s));

#else

    EXPECT_TRUE(!error);
    EXPECT_EQ(0U, result->transferred());

    io_buf->reset();
    socket.async_receive(std::move(io_buf));

    io_buf = context.get();
    ASSERT_NE(nullptr, io_buf);
    result = socket_t::async_receive_result(io_buf, error);
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(!error);
    EXPECT_EQ(case_name, to_string(io_buf, result->transferred()));

#endif
  }

#if __sal_os_windows
  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(context.get()),
    std::system_error
  );
#endif
}


TEST_P(datagram_socket, async_receive_empty_buf_immediate_completion)
{
  {
    socket_t::endpoint_t endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    service.associate(socket);

    socket.send_to(sal::make_buf(case_name), endpoint);
    std::this_thread::sleep_for(1ms);

    auto io_buf = context.make_buf();
    io_buf->resize(0);
    socket.async_receive(std::move(io_buf));

    io_buf = context.get();
    ASSERT_NE(nullptr, io_buf);

    std::error_code error;
    auto result = socket_t::async_receive_result(io_buf, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with empty 1st read, 2nd should have nothing
    io_buf->reset();
    socket.async_receive(std::move(io_buf));
    EXPECT_EQ(nullptr, context.get(0s));
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(context.get()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_send_to)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  // send
  auto io_buf = make_buf(case_name);
  io_buf->user_data(1);
  socket.async_send_to(std::move(io_buf), endpoint);

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), socket.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(socket.local_endpoint(), endpoint);

  // async send result
  io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  EXPECT_EQ(1U, io_buf->user_data());
  auto result = socket.async_send_to_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());

  EXPECT_EQ(nullptr, socket.async_receive_from_result(io_buf));
}


TEST_P(datagram_socket, async_send_to_invalid)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);
  socket.close();

  // send
  socket.async_send_to(make_buf(case_name), endpoint);

  // async send result with error
  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  std::error_code error;
  auto result = socket.async_send_to_result(io_buf, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::bad_file_descriptor, error);
  EXPECT_EQ(0U, result->transferred());

  // with exception
  EXPECT_THROW(
    socket.async_send_to_result(io_buf),
    std::system_error
  );
}


TEST_P(datagram_socket, async_send_to_empty)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  // send
  auto io_buf = context.make_buf();
  io_buf->resize(0);
  socket.async_send_to(std::move(io_buf), endpoint);

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(0U, socket.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(socket.local_endpoint(), endpoint);

  // async send result
  io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  auto result = socket.async_send_to_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred());
}


TEST_P(datagram_socket, async_send_to_do_not_route)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  // send
  auto io_buf = make_buf(case_name);
  socket.async_send_to(std::move(io_buf), endpoint, socket.do_not_route);

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), socket.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(socket.local_endpoint(), endpoint);

  // async send result
  io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  auto result = socket.async_send_to_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());
}


TEST_P(datagram_socket, DISABLED_async_send_to_overflow)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  int send_buffer_size;
  socket.get_option(sal::net::send_buffer_size(&send_buffer_size));
  size_t count = (send_buffer_size / sal::net::io_buf_t::max_size()) * 3;

  std::array<std::thread, 2> threads;
  auto expected_io_count = threads.max_size() * count;

  // start receives
  for (auto i = 0U;  i < expected_io_count;  ++i)
  {
    socket.async_receive_from(context.make_buf());
  }

  // send from multiple threads
  for (auto &thread: threads)
  {
    thread = std::thread([this, count, &socket, &endpoint]
    {
      for (auto i = 0U;  i < count;  ++i)
      {
        socket.async_send_to(context.make_buf(), endpoint);
      }
    });
  }

  // dispatch
  size_t sends = 0, receives = 0;
  auto stop_time = std::chrono::steady_clock::now() + 3s;
  while (std::chrono::steady_clock::now() < stop_time)
  {
    std::error_code error;
    if (auto io_buf = context.get(1s))
    {
      if (socket.async_send_to_result(io_buf, error))
      {
        sends++;
      }
      else if (socket.async_receive_from_result(io_buf, error))
      {
        receives++;
      }
      else
      {
        FAIL() << "unexpected result";
      }
      EXPECT_TRUE(!error) << "value=" << error.value();
    }
    if (sends == expected_io_count && receives == expected_io_count)
    {
      break;
    }
  }

  socket.close();
  for (auto &thread: threads)
  {
    thread.join();
  }

  ASSERT_EQ(expected_io_count, sends);
  EXPECT_EQ(expected_io_count, receives);
}


TEST_P(datagram_socket, async_send)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  // send
  socket.connect(endpoint);
  socket.async_send(make_buf(case_name));

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), socket.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(socket.local_endpoint(), endpoint);

  // async send result
  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  auto result = socket.async_send_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());

  EXPECT_EQ(nullptr, socket.async_receive_result(io_buf));
}


TEST_P(datagram_socket, async_send_not_connected)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  // send
  socket.async_send(make_buf(case_name));

  // async send result
  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  std::error_code error;
  auto result = socket.async_send_result(io_buf, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_EQ(0U, result->transferred());

  // with exception
  EXPECT_THROW(
    socket.async_send_result(io_buf),
    std::system_error
  );
}


TEST_P(datagram_socket, async_send_before_shutdown)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  // send
  socket.connect(endpoint);
  socket.async_send(make_buf(case_name));
  socket.shutdown(socket.shutdown_send);

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), socket.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(socket.local_endpoint(), endpoint);

  // async send result
  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  auto result = socket.async_send_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());
}


TEST_P(datagram_socket, async_send_after_shutdown)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);

  socket.connect(endpoint);
  socket.shutdown(socket.shutdown_send);
  socket.async_send(make_buf(case_name));

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  std::error_code error;
  auto result = socket.async_send_result(io_buf, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(sal::net::socket_errc_t::orderly_shutdown, error);

  EXPECT_THROW(
    socket.async_send_result(io_buf),
    std::system_error
  );
}


TEST_P(datagram_socket, async_send_invalid)
{
  socket_t::endpoint_t endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  service.associate(socket);
  socket.close();

  // send
  socket.async_send(make_buf(case_name));

  // async send result with error
  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  std::error_code error;
  auto result = socket.async_send_result(io_buf, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::bad_file_descriptor, error);
  EXPECT_EQ(0U, result->transferred());

  // with exception
  EXPECT_THROW(
    socket.async_send_result(io_buf),
    std::system_error
  );
}


TEST_P(datagram_socket, async_send_empty)
{
  socket_t socket(loopback(GetParam()));
  socket.connect(socket.local_endpoint());
  service.associate(socket);

  // send
  auto io_buf = context.make_buf();
  io_buf->resize(0);
  socket.async_send(std::move(io_buf));

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(0U, socket.receive(sal::make_buf(buf)));

  // async send result
  io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  auto result = socket.async_send_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred());
}


TEST_P(datagram_socket, async_send_do_not_route)
{
  socket_t socket(loopback(GetParam()));
  socket.connect(socket.local_endpoint());
  service.associate(socket);

  // send
  socket.async_send(make_buf(case_name), socket.do_not_route);

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), socket.receive(sal::make_buf(buf)));
  EXPECT_EQ(buf, case_name);

  // async send result
  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);
  auto result = socket.async_send_result(io_buf);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());
}


TEST_P(datagram_socket, DISABLED_async_send_overflow)
{
  socket_t socket(loopback(GetParam()));
  socket.connect(socket.local_endpoint());
  service.associate(socket);

  int send_buffer_size;
  socket.get_option(sal::net::send_buffer_size(&send_buffer_size));
  size_t count = (send_buffer_size / sal::net::io_buf_t::max_size()) * 3;

  std::array<std::thread, 2> threads;
  auto expected_io_count = threads.max_size() * count;

  // start receives
  for (auto i = 0U;  i < expected_io_count;  ++i)
  {
    socket.async_receive(context.make_buf());
  }

  // send from multiple threads
  for (auto &thread: threads)
  {
    thread = std::thread([this, count, &socket]
    {
      for (auto i = 0U;  i < count;  ++i)
      {
        socket.async_send(context.make_buf());
      }
    });
  }

  // dispatch
  size_t sends = 0, receives = 0;
  auto stop_time = std::chrono::steady_clock::now() + 3s;
  while (std::chrono::steady_clock::now() < stop_time)
  {
    std::error_code error;
    if (auto io_buf = context.get(1s))
    {
      if (socket.async_send_result(io_buf, error))
      {
        sends++;
      }
      else if (socket.async_receive_result(io_buf, error))
      {
        receives++;
      }
      else
      {
        FAIL() << "unexpected result";
      }
      EXPECT_TRUE(!error) << "value=" << error.value();
    }
    if (sends == expected_io_count && receives == expected_io_count)
    {
      break;
    }
  }

  socket.close();
  for (auto &thread: threads)
  {
    thread.join();
  }

  ASSERT_EQ(expected_io_count, sends);
  EXPECT_EQ(expected_io_count, receives);
}


#endif // !__sal_os_linux


} // namespace
