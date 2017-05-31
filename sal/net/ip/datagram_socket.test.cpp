#include <sal/net/ip/udp.hpp>
#include <sal/net/common.test.hpp>
#include <thread>


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
};

constexpr sal::net::ip::port_t datagram_socket::port;


INSTANTIATE_TEST_CASE_P(net_ip, datagram_socket,
  ::testing::Values(
    sal::net::ip::udp_t::v4(),
    sal::net::ip::udp_t::v6()
  ),
  sal_test::to_s<sal::net::ip::udp_t>
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
  auto handle = sal::net::socket_base_t::invalid - 1;
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


TEST_P(datagram_socket, async_receive_from)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint = loopback(GetParam());
  socket_t socket(endpoint);
  socket.associate(svc);

  auto io = ctx.make_io();
  io->user_data(1);
  socket.async_receive_from(std::move(io));

  socket.send_to(sal::make_buf(case_name), endpoint);

  io = ctx.poll();
  ASSERT_NE(nullptr, io);
  EXPECT_EQ(1U, io->user_data());

  auto result = socket.async_receive_from_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(endpoint, result->endpoint());
  EXPECT_EQ(case_name, to_s(io, result));

  EXPECT_EQ(nullptr, socket.async_receive_result(io));
}


TEST_P(datagram_socket, async_receive_from_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint = loopback(GetParam());
  socket_t socket(endpoint);
  socket.associate(svc);

  socket.send_to(sal::make_buf(case_name), endpoint);

  auto io = ctx.make_io();
  io->user_data(2);
  socket.async_receive_from(std::move(io));

  io = ctx.poll();
  ASSERT_NE(nullptr, io);
  EXPECT_EQ(2U, io->user_data());

  auto result = socket.async_receive_from_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(endpoint, result->endpoint());
  EXPECT_EQ(case_name, to_s(io, result));
}


TEST_P(datagram_socket, async_receive_from_partially_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  // start three writes and one read
  socket.async_receive_from(ctx.make_io());
  socket.send_to(sal::make_buf(case_name + "_one"), endpoint);
  socket.send_to(sal::make_buf(case_name + "_two"), endpoint);
  socket.send_to(sal::make_buf(case_name + "_three"), endpoint);

  // first read must succeed
  // (and in case of Reactor, fetch poller event)
  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);
  auto result = socket.async_receive_from_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name + "_one", to_s(io, result));

  // launch thread that will "steal" packet 'two'
  auto t = std::thread([&]
  {
    auto ctx1 = svc.make_context();
    socket.async_receive_from(ctx1.make_io());
    auto io = ctx1.poll();
    ASSERT_NE(nullptr, io);
    auto result = socket.async_receive_from_result(io);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(case_name + "_two", to_s(io, result));
    EXPECT_EQ(&ctx1, &io->this_context());
  });
  t.join();

  // start remaining of reads (one will fail because of "stolen" packet)
  socket.async_receive_from(ctx.make_io());
  socket.async_receive_from(ctx.make_io());

  // second this thread's reads third (final) packet
  io = ctx.poll();
  ASSERT_NE(nullptr, io);
  result = socket.async_receive_from_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name + "_three", to_s(io, result));

  // there is pending read but no data anymore
  io = ctx.poll(1ms);
  EXPECT_EQ(nullptr, io);

  // close socket
  socket.close();
  io = ctx.poll(1ms);
  ASSERT_NE(nullptr, io);

  // closing will cancel read
  std::error_code error;
  result = socket.async_receive_from_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred());
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TEST_P(datagram_socket, async_receive_from_invalid)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  socket.async_receive_from(ctx.make_io());
  socket.close();

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = socket.async_receive_from_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
  EXPECT_EQ(0U, result->transferred());

  EXPECT_THROW(
    socket.async_receive_from_result(io),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_from_invalid_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  socket_t socket(loopback(GetParam()));
  socket.associate(svc);

  socket.close();
  socket.async_receive_from(ctx.make_io());

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = socket.async_receive_from_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::bad_file_descriptor, error);
  EXPECT_EQ(0U, result->transferred());

  EXPECT_THROW(
    socket.async_receive_from_result(io),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_from_no_sender)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    socket_t socket(loopback(GetParam()));
    socket.associate(svc);

    socket.async_receive_from(ctx.make_io());

    EXPECT_EQ(nullptr, ctx.try_poll());
    EXPECT_EQ(nullptr, ctx.try_get());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_from_result(ctx.poll()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_from_peek)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  socket.async_receive_from(ctx.make_io(), socket.peek);
  socket.send_to(sal::make_buf(case_name), endpoint);

  // regardless of peek, completion should be removed from queue
  ASSERT_NE(nullptr, ctx.poll());
  EXPECT_EQ(nullptr, ctx.try_poll());
}


TEST_P(datagram_socket, async_receive_from_peek_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  socket.send_to(sal::make_buf(case_name), endpoint);
  socket.async_receive_from(ctx.make_io(), socket.peek);

  // regardless of peek, completion should be removed from queue
  ASSERT_NE(nullptr, ctx.poll());
  EXPECT_EQ(nullptr, ctx.try_poll());
}


TEST_P(datagram_socket, async_receive_from_less_than_send)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    auto endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    socket.associate(svc);

    auto io = ctx.make_io();
    io->resize(case_name.size() / 2);
    socket.async_receive_from(std::move(io));

    socket.send_to(sal::make_buf(case_name), endpoint);

    io = ctx.poll();
    ASSERT_NE(nullptr, io);

    std::error_code error;
    auto result = socket_t::async_receive_from_result(io, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with partial 1st read, 2nd should have nothing
    io->reset();
    socket.async_receive_from(std::move(io));
    EXPECT_EQ(nullptr, ctx.try_poll());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_from_result(ctx.poll()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_from_less_than_send_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    auto endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    socket.associate(svc);

    socket.send_to(sal::make_buf(case_name), endpoint);

    auto io = ctx.make_io();
    io->resize(case_name.size() / 2);
    socket.async_receive_from(std::move(io));

    io = ctx.poll();
    ASSERT_NE(nullptr, io);

    std::error_code error;
    auto result = socket_t::async_receive_from_result(io, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with partial 1st read, 2nd should have nothing
    io->reset();
    socket.async_receive_from(std::move(io));
    EXPECT_EQ(nullptr, ctx.try_poll());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_from_result(ctx.poll()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_from_empty_buf)
{
  // couldn't unify IOCP/epoll/kqueue behaviour without additional syscall
  // but this is weird case anyway, prefer performance over unification

  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    auto endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    socket.associate(svc);

    auto io = ctx.make_io();
    io->resize(0);
    socket.async_receive_from(std::move(io));

    socket.send_to(sal::make_buf(case_name), endpoint);

    io = ctx.poll();
    ASSERT_NE(nullptr, io);

    std::error_code error;
    auto result = socket_t::async_receive_from_result(io, error);
    ASSERT_NE(nullptr, result);

#if __sal_os_darwin

    // 1st succeed immediately with 0B transferred
    EXPECT_TRUE(!error);
    EXPECT_EQ(0U, result->transferred());

    io->reset();
    socket.async_receive_from(std::move(io));

    // 2nd succeeds with originally sent packet
    io = ctx.poll();
    ASSERT_NE(nullptr, io);
    result = socket_t::async_receive_from_result(io, error);
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(!error);
    EXPECT_EQ(case_name, to_s(io, result));

#else

    // 1st receive is empty (because buf is empty)
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // buf 2nd receive still have nothing
    io->reset();
    socket.async_receive_from(std::move(io));
    EXPECT_EQ(nullptr, ctx.try_poll());

#endif
  }

#if !__sal_os_darwin
  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_from_result(ctx.poll()),
    std::system_error
  );
#endif
}


TEST_P(datagram_socket, async_receive_from_empty_buf_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    auto endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    socket.associate(svc);

    socket.send_to(sal::make_buf(case_name), endpoint);
    std::this_thread::sleep_for(1ms);

    auto io = ctx.make_io();
    io->resize(0);
    socket.async_receive_from(std::move(io));

    io = ctx.poll();
    ASSERT_NE(nullptr, io);

    std::error_code error;
    auto result = socket_t::async_receive_from_result(io, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with empty 1st read, 2nd should have nothing
    io->reset();
    socket.async_receive_from(std::move(io));
    EXPECT_EQ(nullptr, ctx.try_poll());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_from_result(ctx.poll()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  socket.async_receive(ctx.make_io());
  socket.send_to(sal::make_buf(case_name), endpoint);

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = socket.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));

  EXPECT_EQ(nullptr, socket.async_receive_from_result(io));
}


TEST_P(datagram_socket, async_receive_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  socket.send_to(sal::make_buf(case_name), endpoint);
  socket.async_receive(ctx.make_io());

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = socket.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));
}


TEST_P(datagram_socket, async_receive_connected)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  socket_t socket(loopback(GetParam()));
  socket.connect(socket.local_endpoint());
  socket.associate(svc);

  socket.async_receive(ctx.make_io());
  socket.send(sal::make_buf(case_name));

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = socket.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));
}


TEST_P(datagram_socket, async_receive_connected_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  socket_t socket(loopback(GetParam()));
  socket.connect(socket.local_endpoint());
  socket.associate(svc);

  socket.send(sal::make_buf(case_name));
  socket.async_receive(ctx.make_io());

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  auto result = socket.async_receive_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name, to_s(io, result));
}


TEST_P(datagram_socket, async_receive_connected_elsewhere)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    socket_t receiver(GetParam()), sender(GetParam());
    receiver.associate(svc);

    receiver.connect(loopback(GetParam()));
    receiver.async_receive(ctx.make_io());
    sender.send_to(sal::make_buf(case_name), receiver.local_endpoint());

    // must be ignored if from elsewehere than connected
    EXPECT_EQ(nullptr, ctx.try_poll());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(ctx.poll()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_connected_elsewhere_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    socket_t receiver(GetParam()), sender(GetParam());
    receiver.associate(svc);

    receiver.connect(loopback(GetParam()));
    sender.send_to(sal::make_buf(case_name), receiver.local_endpoint());
    receiver.async_receive(ctx.make_io());

    // must be ignored if from elsewehere than connected
    EXPECT_EQ(nullptr, ctx.try_poll());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(ctx.poll()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_invalid)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  socket_t socket(loopback(GetParam()));
  socket.associate(svc);

  socket.async_receive(ctx.make_io());
  socket.close();

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = socket.async_receive_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
  EXPECT_EQ(0U, result->transferred());

  EXPECT_THROW(socket.async_receive_result(io), std::system_error);
}


TEST_P(datagram_socket, async_receive_invalid_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  socket_t socket(loopback(GetParam()));
  socket.associate(svc);

  socket.close();
  socket.async_receive(ctx.make_io());

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = socket.async_receive_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::bad_file_descriptor, error);
  EXPECT_EQ(0U, result->transferred());

  EXPECT_THROW(socket.async_receive_result(io), std::system_error);
}


TEST_P(datagram_socket, async_receive_no_sender)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    socket_t socket(loopback(GetParam()));
    socket.associate(svc);

    socket.async_receive(ctx.make_io());

    EXPECT_EQ(nullptr, ctx.try_get());
    EXPECT_EQ(nullptr, ctx.try_poll());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(ctx.poll()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_peek)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  socket.async_receive(ctx.make_io(), socket.peek);
  socket.send_to(sal::make_buf(case_name), endpoint);

  // regardless of peek, completion should be removed from queue
  ASSERT_NE(nullptr, ctx.poll());
  EXPECT_EQ(nullptr, ctx.try_poll());
}


TEST_P(datagram_socket, async_receive_peek_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  socket.send_to(sal::make_buf(case_name), endpoint);
  socket.async_receive(ctx.make_io(), socket.peek);

  // regardless of peek, completion should be removed from queue
  ASSERT_NE(nullptr, ctx.poll());
  EXPECT_EQ(nullptr, ctx.try_poll());
}


TEST_P(datagram_socket, async_receive_less_than_send)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    auto endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    socket.associate(svc);

    auto io = ctx.make_io();
    io->resize(case_name.size() / 2);
    socket.async_receive(std::move(io));

    socket.send_to(sal::make_buf(case_name), endpoint);

    io = ctx.poll();
    ASSERT_NE(nullptr, io);

    std::error_code error;
    auto result = socket_t::async_receive_result(io, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with partial 1st read, 2nd should have nothing
    io->reset();
    socket.async_receive(std::move(io));
    EXPECT_EQ(nullptr, ctx.try_poll());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(ctx.poll()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_less_than_send_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    auto endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    socket.associate(svc);

    socket.send_to(sal::make_buf(case_name), endpoint);

    auto io = ctx.make_io();
    io->resize(case_name.size() / 2);
    socket.async_receive(std::move(io));

    io = ctx.poll();
    ASSERT_NE(nullptr, io);

    std::error_code error;
    auto result = socket_t::async_receive_result(io, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with partial 1st read, 2nd should have nothing
    io->reset();
    socket.async_receive(std::move(io));
    EXPECT_EQ(nullptr, ctx.try_poll());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(ctx.poll()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_receive_empty_buf)
{
  // couldn't unify IOCP/kqueue behaviour without additional syscall
  // but this is weird case anyway, prefer performance over unification

  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    auto endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    socket.associate(svc);

    auto io = ctx.make_io();
    io->resize(0);
    socket.async_receive(std::move(io));

    socket.send_to(sal::make_buf(case_name), endpoint);

    io = ctx.poll();
    ASSERT_NE(nullptr, io);

    std::error_code error;
    auto result = socket_t::async_receive_result(io, error);
    ASSERT_NE(nullptr, result);

#if __sal_os_darwin

    EXPECT_TRUE(!error);
    EXPECT_EQ(0U, result->transferred());

    io->reset();
    socket.async_receive(std::move(io));

    io = ctx.poll();
    ASSERT_NE(nullptr, io);
    result = socket_t::async_receive_result(io, error);
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(!error);
    EXPECT_EQ(case_name, to_s(io, result));

#else

    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with empty 1st read, 2nd should have nothing
    io->reset();
    socket.async_receive(std::move(io));
    EXPECT_EQ(nullptr, ctx.try_poll());

#endif
  }

#if !__sal_os_darwin
  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(ctx.poll()),
    std::system_error
  );
#endif
}


TEST_P(datagram_socket, async_receive_empty_buf_immediate_completion)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  {
    auto endpoint(loopback(GetParam()));
    socket_t socket(endpoint);
    socket.associate(svc);

    socket.send_to(sal::make_buf(case_name), endpoint);
    std::this_thread::sleep_for(1ms);

    auto io = ctx.make_io();
    io->resize(0);
    socket.async_receive(std::move(io));

    io = ctx.poll();
    ASSERT_NE(nullptr, io);

    std::error_code error;
    auto result = socket_t::async_receive_result(io, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::message_size, error);
    EXPECT_EQ(0U, result->transferred());

    // even with empty 1st read, 2nd should have nothing
    io->reset();
    socket.async_receive(std::move(io));
    EXPECT_EQ(nullptr, ctx.try_poll());
  }

  // error from closed socket still in context
  EXPECT_THROW(
    socket_t::async_receive_result(ctx.poll()),
    std::system_error
  );
}


TEST_P(datagram_socket, async_send_to)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  // send
  auto io = from_s(ctx, case_name);
  io->user_data(1);
  socket.async_send_to(std::move(io), endpoint);

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), socket.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(socket.local_endpoint(), endpoint);

  // async send result
  io = ctx.poll();
  ASSERT_NE(nullptr, io);
  EXPECT_EQ(1U, io->user_data());
  auto result = socket.async_send_to_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());

  EXPECT_EQ(nullptr, socket.async_receive_from_result(io));
}


TEST_P(datagram_socket, async_send_to_invalid)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);
  socket.close();

  // send
  socket.async_send_to(from_s(ctx, case_name), endpoint);

  // async send result with error
  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);
  std::error_code error;
  auto result = socket.async_send_to_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::bad_file_descriptor, error);
  EXPECT_EQ(0U, result->transferred());

  // with exception
  EXPECT_THROW(
    socket.async_send_to_result(io),
    std::system_error
  );
}


TEST_P(datagram_socket, async_send_to_empty)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  // send
  auto io = ctx.make_io();
  io->resize(0);
  socket.async_send_to(std::move(io), endpoint);

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(0U, socket.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(socket.local_endpoint(), endpoint);

  // async send result
  io = ctx.poll();
  ASSERT_NE(nullptr, io);
  auto result = socket.async_send_to_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred());
}


TEST_P(datagram_socket, async_send_to_do_not_route)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  // send
  auto io = from_s(ctx, case_name);
  socket.async_send_to(std::move(io), endpoint, socket.do_not_route);

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), socket.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(socket.local_endpoint(), endpoint);

  // async send result
  io = ctx.poll();
  ASSERT_NE(nullptr, io);
  auto result = socket.async_send_to_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());
}


TEST_P(datagram_socket, async_send_to_overflow)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  int send_buffer_size = 4*1024;
  socket.set_option(sal::net::send_buffer_size(send_buffer_size));
  socket.get_option(sal::net::send_buffer_size(&send_buffer_size));

  std::array<std::thread, 4> threads;
  auto per_thread_sends = (send_buffer_size / sal::net::async_service_t::io_t::max_size()) * 16;
  auto total_sends = per_thread_sends * threads.max_size();

  // receives
  for (auto i = 0U;  i != total_sends;  ++i)
  {
    socket.async_receive_from(ctx.make_io());
  }

  std::atomic<size_t> sends{}, receives{};
  for (auto &thread: threads)
  {
    thread = std::thread([&]
    {
      auto ctx = svc.make_context();

      // sends
      for (auto i = 0U;  i != per_thread_sends;  ++i)
      {
        socket.async_send_to(ctx.make_io(), endpoint);
      }

      // completions
      auto stop_time = std::chrono::steady_clock::now() + 250ms;
      while (std::chrono::steady_clock::now() < stop_time)
      {
        std::error_code error;
        if (auto io = ctx.poll(10ms, error))
        {
          EXPECT_TRUE(!error) << "context error: " << error.message();

          if (socket.async_send_to_result(io, error))
          {
            sends++;
          }
          else if (socket.async_receive_from_result(io, error))
          {
            receives++;
          }
          else
          {
            FAIL() << "unexpected result";
          }
          EXPECT_TRUE(!error) << "socket error: " << error.message();

          if (sends == total_sends && receives == total_sends)
          {
            break;
          }
        }
      }
    });
  }

  for (auto &thread: threads)
  {
    thread.join();
  }

  // must send everything
  ASSERT_EQ(total_sends, sends);

  // but may drop some
  // (randomly checking for at least 75%)
  EXPECT_GT(total_sends, receives * 3 / 4);
}


TEST_P(datagram_socket, async_send)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  // send
  socket.connect(endpoint);
  socket.async_send(from_s(ctx, case_name));

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), socket.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(socket.local_endpoint(), endpoint);

  // async send result
  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);
  auto result = socket.async_send_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());

  EXPECT_EQ(nullptr, socket.async_receive_result(io));
}


TEST_P(datagram_socket, async_send_not_connected)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  // send
  socket.async_send(from_s(ctx, case_name));

  // async send result
  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = socket.async_send_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_EQ(0U, result->transferred());

  // with exception
  EXPECT_THROW(
    socket.async_send_result(io),
    std::system_error
  );
}


TEST_P(datagram_socket, async_send_before_shutdown)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  // send
  socket.connect(endpoint);
  socket.async_send(from_s(ctx, case_name));
  socket.shutdown(socket.shutdown_send);

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), socket.receive_from(sal::make_buf(buf), endpoint));
  EXPECT_EQ(buf, case_name);
  EXPECT_EQ(socket.local_endpoint(), endpoint);

  // async send result
  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);
  auto result = socket.async_send_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());
}


TEST_P(datagram_socket, async_send_after_shutdown)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);

  socket.connect(endpoint);
  socket.shutdown(socket.shutdown_send);
  socket.async_send(from_s(ctx, case_name));

  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = socket.async_send_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::broken_pipe, error);

  EXPECT_THROW(
    socket.async_send_result(io),
    std::system_error
  );
}


TEST_P(datagram_socket, async_send_invalid)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  auto endpoint(loopback(GetParam()));
  socket_t socket(endpoint);
  socket.associate(svc);
  socket.close();

  // send
  socket.async_send(from_s(ctx, case_name));

  // async send result with error
  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);
  std::error_code error;
  auto result = socket.async_send_result(io, error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::bad_file_descriptor, error);
  EXPECT_EQ(0U, result->transferred());

  // with exception
  EXPECT_THROW(
    socket.async_send_result(io),
    std::system_error
  );
}


TEST_P(datagram_socket, async_send_empty)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  socket_t socket(loopback(GetParam()));
  socket.connect(socket.local_endpoint());
  socket.associate(svc);

  // send
  auto io = ctx.make_io();
  io->resize(0);
  socket.async_send(std::move(io));

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(0U, socket.receive(sal::make_buf(buf)));

  // async send result
  io = ctx.poll();
  ASSERT_NE(nullptr, io);
  auto result = socket.async_send_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred());
}


TEST_P(datagram_socket, async_send_do_not_route)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  socket_t socket(loopback(GetParam()));
  socket.connect(socket.local_endpoint());
  socket.associate(svc);

  // send
  socket.async_send(from_s(ctx, case_name), socket.do_not_route);

  // receive
  char buf[1024];
  std::memset(buf, '\0', sizeof(buf));
  EXPECT_EQ(case_name.size(), socket.receive(sal::make_buf(buf)));
  EXPECT_EQ(buf, case_name);

  // async send result
  auto io = ctx.poll();
  ASSERT_NE(nullptr, io);
  auto result = socket.async_send_result(io);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(case_name.size(), result->transferred());
}


TEST_P(datagram_socket, async_send_overflow)
{
  sal::net::async_service_t svc;
  auto ctx = svc.make_context();

  socket_t socket(loopback(GetParam()));
  socket.connect(socket.local_endpoint());
  socket.associate(svc);

  int send_buffer_size = 4*1024;
  socket.set_option(sal::net::send_buffer_size(send_buffer_size));
  socket.get_option(sal::net::send_buffer_size(&send_buffer_size));

  std::array<std::thread, 4> threads;
  size_t per_thread_sends = (send_buffer_size / sal::net::async_service_t::io_t::max_size()) * 16;
  size_t total_sends = per_thread_sends * threads.max_size();

  // receives
  for (auto i = 0U;  i != total_sends;  ++i)
  {
    socket.async_receive(ctx.make_io());
  }

  std::atomic<size_t> sends{}, receives{};
  for (auto &thread: threads)
  {
    thread = std::thread([&]
    {
      auto ctx = svc.make_context();

      // sends
      for (auto i = 0U;  i != per_thread_sends;  ++i)
      {
        socket.async_send(ctx.make_io());
      }

      // completions
      auto stop_time = std::chrono::steady_clock::now() + 250ms;
      while (std::chrono::steady_clock::now() < stop_time)
      {
        std::error_code error;
        if (auto io = ctx.poll(10ms, error))
        {
          EXPECT_TRUE(!error) << "context error: " << error.message();

          if (socket.async_send_result(io, error))
          {
            sends++;
          }
          else if (socket.async_receive_result(io, error))
          {
            receives++;
          }
          else
          {
            FAIL() << "unexpected result";
          }
          EXPECT_TRUE(!error) << "socket error: " << error.message();

          if (sends == total_sends && receives == total_sends)
          {
            break;
          }
        }
      }
    });
  }

  for (auto &thread: threads)
  {
    thread.join();
  }

  // must send everything
  ASSERT_EQ(total_sends, sends);

  // but may drop some
  // (randomly checking for at least 75%)
  EXPECT_GT(total_sends, receives * 3 / 4);
}


} // namespace
