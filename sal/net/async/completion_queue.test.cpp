#include <sal/net/async/completion_queue.hpp>
#include <sal/net/async/service.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/net/common.test.hpp>
#include <thread>


namespace {


using namespace std::chrono_literals;
using sal_test::to_view;

using protocol_t = sal::net::ip::udp_t;
using endpoint_t = protocol_t::endpoint_t;
using socket_t = protocol_t::socket_t;


struct net_async_completion_queue
  : public sal_test::fixture
{
  sal::net::async::service_t service{};
  sal::net::async::completion_queue_t queue{service};

  const endpoint_t endpoint{sal::net::ip::address_v4_t::loopback, 8195};
  socket_t a{endpoint}, b{protocol_t::v4};


  void SetUp ()
  {
    a.associate(service);
    b.associate(service);
    b.connect(endpoint);
  }


  void send (socket_t &socket, const std::string &data)
  {
    socket.send(data);
    std::this_thread::sleep_for(1ms);
  }
};


TEST_F(net_async_completion_queue, make_io) //{{{1
{
  auto io = queue.make_io();
  EXPECT_EQ(nullptr, io->context<int>());
}


TEST_F(net_async_completion_queue, make_io_with_context) //{{{1
{
  int io_ctx;
  auto io = queue.make_io(&io_ctx);
  EXPECT_EQ(&io_ctx, io->context<int>());
}


TEST_F(net_async_completion_queue, wait_for) //{{{1
{
  a.start_receive(queue.make_io());
  send(b, case_name);

  EXPECT_TRUE(queue.wait_for(1s));
  auto io = queue.try_get();
  ASSERT_NE(nullptr, io);

  auto event = io->get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, event);
  EXPECT_EQ(case_name, to_view(io, event));
}


TEST_F(net_async_completion_queue, wait) //{{{1
{
  a.start_receive(queue.make_io());
  send(b, case_name);

  EXPECT_TRUE(queue.wait());
  auto io = queue.try_get();
  ASSERT_NE(nullptr, io);

  auto event = io->get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, event);
  EXPECT_EQ(case_name, to_view(io, event));
}


TEST_F(net_async_completion_queue, poll) //{{{1
{
  a.start_receive(queue.make_io());
  send(b, case_name);

  EXPECT_TRUE(queue.poll());
  auto io = queue.try_get();
  ASSERT_NE(nullptr, io);

  auto event = io->get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, event);
  EXPECT_EQ(case_name, to_view(io, event));
}


TEST_F(net_async_completion_queue, try_get_with_no_async_io) //{{{1
{
  auto io = queue.try_get();
  EXPECT_EQ(nullptr, io);
}


TEST_F(net_async_completion_queue, try_get_with_immediate_completion) //{{{1
{
  send(b, case_name);
  a.start_receive(queue.make_io());

  auto io = queue.try_get();
  ASSERT_NE(nullptr, io);

  auto event = io->get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, event);
  EXPECT_EQ(case_name, to_view(io, event));
}


TEST_F(net_async_completion_queue, try_get_with_delayed_completion) //{{{1
{
  a.start_receive(queue.make_io());
  send(b, case_name);

  auto io = queue.try_get();
  EXPECT_EQ(nullptr, io);

  EXPECT_TRUE(queue.poll());
  io = queue.try_get();
  ASSERT_NE(nullptr, io);

  auto event = io->get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, event);
  EXPECT_EQ(case_name, to_view(io, event));
}


TEST_F(net_async_completion_queue, dtor_moves_completed_io_to_service) //{{{1
{
  {
    sal::net::async::completion_queue_t local_queue{service};
    a.start_receive(local_queue.make_io());
    send(b, case_name);
    EXPECT_TRUE(local_queue.poll());
  }

  EXPECT_FALSE(queue.poll());

  auto io = queue.try_get();
  ASSERT_NE(nullptr, io);

  auto event = io->get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, event);
  EXPECT_EQ(case_name, to_view(io, event));
}


TEST_F(net_async_completion_queue, send_skip_completion_queue_immediate) //{{{1
{
  a.start_receive(queue.make_io());
  EXPECT_FALSE(queue.poll());
  EXPECT_EQ(nullptr, queue.try_get());

  auto io = queue.make_io();
  io->skip_completion_notification(true);
  EXPECT_TRUE(io->skip_completion_notification());

  io->resize(case_name.size());
  std::memcpy(io->data(), case_name.data(), case_name.size());
  b.start_send(std::move(io));

  EXPECT_TRUE(queue.wait());
  io = queue.try_get();
  ASSERT_NE(nullptr, io);

  auto event = io->get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, event);
  EXPECT_EQ(case_name, to_view(io, event));

  EXPECT_EQ(nullptr, queue.try_get());
}


TEST_F(net_async_completion_queue, send_skip_completion_queue_delayed) //{{{1
{
  auto io = queue.make_io();
  io->skip_completion_notification(true);
  EXPECT_TRUE(io->skip_completion_notification());

  io->resize(case_name.size());
  std::memcpy(io->data(), case_name.data(), case_name.size());
  b.start_send(std::move(io));

  EXPECT_EQ(nullptr, queue.try_get());
  EXPECT_FALSE(queue.poll());
  EXPECT_EQ(nullptr, queue.try_get());

  std::this_thread::sleep_for(1ms);
  a.start_receive(queue.make_io());
  io = queue.try_get();
  ASSERT_NE(nullptr, io);

  auto event = io->get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, event);
  EXPECT_EQ(case_name, to_view(io, event));
}


TEST_F(net_async_completion_queue, receive_skip_completion_queue_immediate) //{{{1
{
  send(b, case_name);

  auto io = queue.make_io();
  io->skip_completion_notification(true);
  EXPECT_TRUE(io->skip_completion_notification());
  a.start_receive(std::move(io));

  EXPECT_FALSE(queue.poll());

  io = queue.try_get();
  EXPECT_EQ(nullptr, io);
}


TEST_F(net_async_completion_queue, receive_skip_completion_queue_delayed) //{{{1
{
  auto io = queue.make_io();
  io->skip_completion_notification(true);
  EXPECT_TRUE(io->skip_completion_notification());
  a.start_receive(std::move(io));
  EXPECT_FALSE(queue.poll());

  send(b, case_name);

  EXPECT_TRUE(queue.poll());
  EXPECT_EQ(nullptr, queue.try_get());
}


//}}}1


} // namespace
