#include <sal/net/async/service.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/common.test.hpp>


namespace {


using namespace std::chrono_literals;
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

    // just for case, depends on IP stack implementation
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1ms);
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

TYPED_TEST_CASE(net_async_datagram_socket, address_types, );


template <typename Result>
inline std::string_view to_view (sal::net::async::io_t &io,
  const Result *result) noexcept
{
  return {reinterpret_cast<const char *>(io.data()), result->transferred};
}


TYPED_TEST(net_async_datagram_socket, receive_from_async) //{{{1
{
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  ASSERT_EQ(nullptr, io.template get_if<socket_t::receive_t>());

  auto result = io.template get_if<socket_t::receive_from_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
  EXPECT_EQ(TestFixture::test_socket.local_endpoint(), result->remote_endpoint);
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_after_send) //{{{1
{
  TestFixture::send(TestFixture::case_name);
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  auto result = io.template get_if<socket_t::receive_from_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
  EXPECT_EQ(TestFixture::test_socket.local_endpoint(), result->remote_endpoint);
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_with_context) //{{{1
{
  int socket_ctx = 1, io_ctx = 2;
  TestFixture::socket.context(&socket_ctx);

  TestFixture::socket.receive_from_async(TestFixture::service.make_io(&io_ctx));
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  EXPECT_EQ(&io_ctx, io.template context<int>());
  EXPECT_EQ(&socket_ctx, io.template socket_context<int>());

  auto result = io.template get_if<socket_t::receive_from_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
  EXPECT_EQ(TestFixture::test_socket.local_endpoint(), result->remote_endpoint);
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_canceled_on_close) //{{{1
{
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());
  TestFixture::socket.close();

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_from_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_no_sender) //{{{1
{
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());
  EXPECT_TRUE(!TestFixture::service.try_poll());
  EXPECT_TRUE(!TestFixture::service.try_get());
  TestFixture::socket.close();

  auto io = TestFixture::service.try_poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_from_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_peek) //{{{1
{
  TestFixture::socket.receive_from_async(TestFixture::service.make_io(), socket_t::peek);
  TestFixture::send(TestFixture::case_name);

  // regardless of peek, completion should be removed from queue
  EXPECT_FALSE(!TestFixture::service.poll());
  EXPECT_TRUE(!TestFixture::service.try_poll());
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_peek_after_send) //{{{1
{
  TestFixture::send(TestFixture::case_name);
  TestFixture::socket.receive_from_async(TestFixture::service.make_io(), socket_t::peek);

  // regardless of peek, completion should be removed from queue
  EXPECT_FALSE(!TestFixture::service.poll());
  EXPECT_TRUE(!TestFixture::service.try_poll());
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_less_than_send) //{{{1
{
  std::string_view data{
    TestFixture::case_name.data(),
    TestFixture::case_name.size() / 2,
  };

  auto io = TestFixture::service.make_io();
  io.resize(data.size());
  TestFixture::socket.receive_from_async(std::move(io));
  TestFixture::send(TestFixture::case_name);

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_from_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(data.size(), result->transferred);
  EXPECT_EQ(data, to_view(io, result));

  // even with partial read, 2nd should have nothing
  io.reset();
  TestFixture::socket.receive_from_async(std::move(io));
  io = TestFixture::service.try_poll();
  EXPECT_TRUE(!io);
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_after_send_less_than_send) //{{{1
{
  TestFixture::send(TestFixture::case_name);

  std::string_view data{
    TestFixture::case_name.data(),
    TestFixture::case_name.size() / 2,
  };

  auto io = TestFixture::service.make_io();
  io.resize(data.size());
  TestFixture::socket.receive_from_async(std::move(io));

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_from_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(data.size(), result->transferred);
  EXPECT_EQ(data, to_view(io, result));

  // even with partial read, 2nd should have nothing
  io.reset();
  TestFixture::socket.receive_from_async(std::move(io));
  io = TestFixture::service.try_poll();
  EXPECT_TRUE(!io);
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_empty_buf) //{{{1
{
  // can't unify IOCP/epoll/kqueue behavior without additional syscall
  // but this is weird case anyway, prefer performance over unification

  auto io = TestFixture::service.make_io();
  io.resize(0);
  TestFixture::socket.receive_from_async(std::move(io));
  TestFixture::send(TestFixture::case_name);

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_from_t>(error);
  ASSERT_NE(nullptr, result);

#if __sal_os_macos

  // 1st attempt succeeds immediately, with 0B transferred but leaving data in
  // OS socket buffer
  EXPECT_TRUE(!error);
  EXPECT_EQ(0U, result->transferred);

  io.reset();
  TestFixture::socket.receive_from_async(std::move(io));

  // 2nd attempt with appropriately sized buffer succeeds with originally sent
  // data
  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);
  result = io.template get_if<socket_t::receive_from_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));

#else

  // 1st attempt fails, OS drops data from socket buffer
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(0U, result->transferred);

  // 2st attempt fails with no data
  io.reset();
  TestFixture::socket.receive_from_async(std::move(io));
  io = TestFixture::service.try_poll();
  EXPECT_TRUE(!io);

#endif
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_after_send_empty_buf) //{{{1
{
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::service.make_io();
  io.resize(0);
  TestFixture::socket.receive_from_async(std::move(io));

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_from_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(0U, result->transferred);
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_interleaved) //{{{1
{
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());

  TestFixture::send("one");
  TestFixture::send("two");
  TestFixture::send("three");

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);
  auto result = io.template get_if<socket_t::receive_from_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ("one", to_view(io, result));

  std::thread([&]
  {
    auto i = TestFixture::service.poll();
    ASSERT_FALSE(!i);
    auto r = i.template get_if<socket_t::receive_from_t>();
    ASSERT_NE(nullptr, r);
    EXPECT_EQ("two", to_view(i, r));
  }).join();

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);
  result = io.template get_if<socket_t::receive_from_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ("three", to_view(io, result));
}


TYPED_TEST(net_async_datagram_socket, receive_from_async_interleaved_after_send) //{{{1
{
  TestFixture::send("one");
  TestFixture::send("two");
  TestFixture::send("three");

  TestFixture::socket.receive_from_async(TestFixture::service.make_io());
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());
  TestFixture::socket.receive_from_async(TestFixture::service.make_io());

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);
  auto result = io.template get_if<socket_t::receive_from_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ("one", to_view(io, result));

  std::thread([&]
  {
    auto i = TestFixture::service.poll();
    ASSERT_FALSE(!i);
    auto r = i.template get_if<socket_t::receive_from_t>();
    ASSERT_NE(nullptr, r);
    EXPECT_EQ("two", to_view(i, r));
  }).join();

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);
  result = io.template get_if<socket_t::receive_from_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ("three", to_view(io, result));
}


//}}}1


TYPED_TEST(net_async_datagram_socket, receive_async) //{{{1
{
  TestFixture::socket.receive_async(TestFixture::service.make_io());
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  ASSERT_EQ(nullptr, io.template get_if<socket_t::receive_from_t>());

  auto result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_datagram_socket, receive_async_after_send) //{{{1
{
  TestFixture::send(TestFixture::case_name);
  TestFixture::socket.receive_async(TestFixture::service.make_io());

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  auto result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_datagram_socket, receive_async_with_context) //{{{1
{
  int socket_ctx = 1, io_ctx = 2;
  TestFixture::socket.context(&socket_ctx);

  TestFixture::socket.receive_async(TestFixture::service.make_io(&io_ctx));
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  EXPECT_EQ(&io_ctx, io.template context<int>());
  EXPECT_EQ(&socket_ctx, io.template socket_context<int>());

  auto result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));
}


TYPED_TEST(net_async_datagram_socket, receive_async_canceled_on_close) //{{{1
{
  TestFixture::socket.receive_async(TestFixture::service.make_io());
  TestFixture::socket.close();

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_datagram_socket, receive_async_no_sender) //{{{1
{
  TestFixture::socket.receive_async(TestFixture::service.make_io());
  EXPECT_TRUE(!TestFixture::service.try_poll());
  EXPECT_TRUE(!TestFixture::service.try_get());
  TestFixture::socket.close();

  auto io = TestFixture::service.try_poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_datagram_socket, receive_async_peek) //{{{1
{
  TestFixture::socket.receive_async(TestFixture::service.make_io(), socket_t::peek);
  TestFixture::send(TestFixture::case_name);

  // regardless of peek, completion should be removed from queue
  EXPECT_FALSE(!TestFixture::service.poll());
  EXPECT_TRUE(!TestFixture::service.try_poll());
}


TYPED_TEST(net_async_datagram_socket, receive_async_peek_after_send) //{{{1
{
  TestFixture::send(TestFixture::case_name);
  TestFixture::socket.receive_async(TestFixture::service.make_io(), socket_t::peek);

  // regardless of peek, completion should be removed from queue
  EXPECT_FALSE(!TestFixture::service.poll());
  EXPECT_TRUE(!TestFixture::service.try_poll());
}


TYPED_TEST(net_async_datagram_socket, receive_async_less_than_send) //{{{1
{
  std::string_view data{
    TestFixture::case_name.data(),
    TestFixture::case_name.size() / 2,
  };

  auto io = TestFixture::service.make_io();
  io.resize(data.size());
  TestFixture::socket.receive_async(std::move(io));
  TestFixture::send(TestFixture::case_name);

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(data.size(), result->transferred);
  EXPECT_EQ(data, to_view(io, result));

  // even with partial read, 2nd should have nothing
  io.reset();
  TestFixture::socket.receive_async(std::move(io));
  io = TestFixture::service.try_poll();
  EXPECT_TRUE(!io);
}


TYPED_TEST(net_async_datagram_socket, receive_async_after_send_less_than_send) //{{{1
{
  TestFixture::send(TestFixture::case_name);

  std::string_view data{
    TestFixture::case_name.data(),
    TestFixture::case_name.size() / 2,
  };

  auto io = TestFixture::service.make_io();
  io.resize(data.size());
  TestFixture::socket.receive_async(std::move(io));

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(data.size(), result->transferred);
  EXPECT_EQ(data, to_view(io, result));

  // even with partial read, 2nd should have nothing
  io.reset();
  TestFixture::socket.receive_async(std::move(io));
  io = TestFixture::service.try_poll();
  EXPECT_TRUE(!io);
}


TYPED_TEST(net_async_datagram_socket, receive_async_empty_buf) //{{{1
{
  // can't unify IOCP/epoll/kqueue behavior without additional syscall
  // but this is weird case anyway, prefer performance over unification

  auto io = TestFixture::service.make_io();
  io.resize(0);
  TestFixture::socket.receive_async(std::move(io));
  TestFixture::send(TestFixture::case_name);

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);

#if __sal_os_macos

  // 1st attempt succeeds immediately, with 0B transferred but leaving data in
  // OS socket buffer
  EXPECT_TRUE(!error);
  EXPECT_EQ(0U, result->transferred);

  io.reset();
  TestFixture::socket.receive_async(std::move(io));

  // 2nd attempt with appropriately sized buffer succeeds with originally sent
  // data
  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);
  result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name, to_view(io, result));

#else

  // 1st attempt fails, OS drops data from socket buffer
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(0U, result->transferred);

  // 2st attempt fails with no data
  io.reset();
  TestFixture::socket.receive_async(std::move(io));
  io = TestFixture::service.try_poll();
  EXPECT_TRUE(!io);

#endif
}


TYPED_TEST(net_async_datagram_socket, receive_async_after_send_empty_buf) //{{{1
{
  TestFixture::send(TestFixture::case_name);

  auto io = TestFixture::service.make_io();
  io.resize(0);
  TestFixture::socket.receive_async(std::move(io));

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  std::error_code error;
  auto result = io.template get_if<socket_t::receive_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::message_size, error);
  EXPECT_EQ(0U, result->transferred);
}


TYPED_TEST(net_async_datagram_socket, receive_async_interleaved) //{{{1
{
  TestFixture::socket.receive_async(TestFixture::service.make_io());
  TestFixture::socket.receive_async(TestFixture::service.make_io());
  TestFixture::socket.receive_async(TestFixture::service.make_io());

  TestFixture::send("one");
  TestFixture::send("two");
  TestFixture::send("three");

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);
  auto result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ("one", to_view(io, result));

  std::thread([&]
  {
    auto i = TestFixture::service.poll();
    ASSERT_FALSE(!i);
    auto r = i.template get_if<socket_t::receive_t>();
    ASSERT_NE(nullptr, r);
    EXPECT_EQ("two", to_view(i, r));
  }).join();

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);
  result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ("three", to_view(io, result));
}


TYPED_TEST(net_async_datagram_socket, receive_async_interleaved_after_send) //{{{1
{
  TestFixture::send("one");
  TestFixture::send("two");
  TestFixture::send("three");

  TestFixture::socket.receive_async(TestFixture::service.make_io());
  TestFixture::socket.receive_async(TestFixture::service.make_io());
  TestFixture::socket.receive_async(TestFixture::service.make_io());

  auto io = TestFixture::service.poll();
  ASSERT_FALSE(!io);
  auto result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ("one", to_view(io, result));

  std::thread([&]
  {
    auto i = TestFixture::service.poll();
    ASSERT_FALSE(!i);
    auto r = i.template get_if<socket_t::receive_t>();
    ASSERT_NE(nullptr, r);
    EXPECT_EQ("two", to_view(i, r));
  }).join();

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);
  result = io.template get_if<socket_t::receive_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ("three", to_view(io, result));
}


//}}}1


TYPED_TEST(net_async_datagram_socket, send_to_async) //{{{1
{
  auto io = TestFixture::service.make_io();
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.send_to_async(std::move(io),
    TestFixture::test_socket.local_endpoint()
  );
  EXPECT_EQ(TestFixture::case_name, TestFixture::receive());

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  auto result = io.template get_if<socket_t::send_to_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name.size(), result->transferred);
}


TYPED_TEST(net_async_datagram_socket, send_to_async_with_context) //{{{1
{
  int socket_ctx = 1, io_ctx = 2;
  TestFixture::socket.context(&socket_ctx);

  auto io = TestFixture::service.make_io(&io_ctx);
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.send_to_async(std::move(io),
    TestFixture::test_socket.local_endpoint()
  );
  EXPECT_EQ(TestFixture::case_name, TestFixture::receive());

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  EXPECT_EQ(&io_ctx, io.template context<int>());
  EXPECT_EQ(&socket_ctx, io.template socket_context<int>());

  auto result = io.template get_if<socket_t::send_to_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name.size(), result->transferred);
}


TYPED_TEST(net_async_datagram_socket, send_to_async_empty_buf) //{{{1
{
  auto io = TestFixture::service.make_io();
  io.resize(0);
  TestFixture::socket.send_to_async(std::move(io),
    TestFixture::test_socket.local_endpoint()
  );

  char buf[1024];
  EXPECT_EQ(0U, TestFixture::test_socket.receive(buf));

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  auto result = io.template get_if<socket_t::send_to_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred);
}


TYPED_TEST(net_async_datagram_socket, send_to_async_overflow) //{{{1
{
  auto max_io_size = sal::net::async::io_t::max_size();

  auto send_buffer_size = static_cast<int>(max_io_size);
  TestFixture::socket.set_option(sal::net::send_buffer_size(send_buffer_size));
  TestFixture::socket.get_option(sal::net::send_buffer_size(&send_buffer_size));

  std::array<std::thread, 4> threads;
  auto per_thread_send_count = (send_buffer_size / max_io_size) * 128;
  auto total_send_count = per_thread_send_count * threads.max_size();

  // receives
  for (auto i = 0U;  i != total_send_count;  ++i)
  {
    TestFixture::socket.receive_from_async(TestFixture::service.make_io());
  }

  std::atomic<size_t> sends{}, receives{};
  for (auto &thread: threads)
  {
    thread = std::thread([&]
    {
      // sends
      for (auto i = 0U;  i < per_thread_send_count;  ++i)
      {
        TestFixture::socket.send_to_async(
          TestFixture::service.make_io(),
          TestFixture::endpoint
        );
      }

      // completions
      const auto deadline = std::chrono::steady_clock::now() + 250ms;
      while (std::chrono::steady_clock::now() < deadline)
      {
        std::error_code error;
        if (auto io = TestFixture::service.poll(10ms, error))
        {
          EXPECT_TRUE(!error) << "poll: " << error.message();

          if (io.template get_if<socket_t::send_to_t>(error))
          {
            sends++;
          }
          else if (io.template get_if<socket_t::receive_from_t>(error))
          {
            receives++;
          }
          else
          {
            FAIL() << "unexpected completion";
          }
          EXPECT_TRUE(!error) << "completion: " << error.message();

          if (sends == total_send_count && receives == sends)
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

  TestFixture::socket.close();

  // must send everything
  EXPECT_EQ(total_send_count, sends);

  // but can miss any number receives depending on IP stack impl
  size_t canceled = 0;
  std::error_code error;
  while (auto io = TestFixture::service.try_poll())
  {
    EXPECT_TRUE(io.template get_if<socket_t::receive_from_t>(error));
    EXPECT_EQ(std::errc::operation_canceled, error);
    canceled++;
  }
  EXPECT_EQ(total_send_count - canceled, receives);
}


//}}}1


TYPED_TEST(net_async_datagram_socket, send_async) //{{{1
{
  TestFixture::socket.connect(TestFixture::test_socket.local_endpoint());

  auto io = TestFixture::service.make_io();
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.send_async(std::move(io));
  EXPECT_EQ(TestFixture::case_name, TestFixture::receive());

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  auto result = io.template get_if<socket_t::send_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name.size(), result->transferred);
}


TYPED_TEST(net_async_datagram_socket, send_async_with_context) //{{{1
{
  TestFixture::socket.connect(TestFixture::test_socket.local_endpoint());

  int socket_ctx = 1, io_ctx = 2;
  TestFixture::socket.context(&socket_ctx);

  auto io = TestFixture::service.make_io(&io_ctx);
  TestFixture::fill(io, TestFixture::case_name);
  TestFixture::socket.send_async(std::move(io));
  EXPECT_EQ(TestFixture::case_name, TestFixture::receive());

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  EXPECT_EQ(&io_ctx, io.template context<int>());
  EXPECT_EQ(&socket_ctx, io.template socket_context<int>());

  auto result = io.template get_if<socket_t::send_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(TestFixture::case_name.size(), result->transferred);
}


TYPED_TEST(net_async_datagram_socket, send_async_empty_buf) //{{{1
{
  TestFixture::socket.connect(TestFixture::test_socket.local_endpoint());

  auto io = TestFixture::service.make_io();
  io.resize(0);
  TestFixture::socket.send_async(std::move(io));

  char buf[1024];
  EXPECT_EQ(0U, TestFixture::test_socket.receive(buf));

  io = TestFixture::service.poll();
  ASSERT_FALSE(!io);

  auto result = io.template get_if<socket_t::send_t>();
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(0U, result->transferred);
}


TYPED_TEST(net_async_datagram_socket, send_async_overflow) //{{{1
{
  TestFixture::socket.connect(TestFixture::test_socket.local_endpoint());

  auto max_io_size = sal::net::async::io_t::max_size();

  auto send_buffer_size = static_cast<int>(max_io_size);
  TestFixture::socket.set_option(sal::net::send_buffer_size(send_buffer_size));
  TestFixture::socket.get_option(sal::net::send_buffer_size(&send_buffer_size));

  std::array<std::thread, 4> threads;
  auto per_thread_send_count = (send_buffer_size / max_io_size) * 128;
  auto total_send_count = per_thread_send_count * threads.max_size();

  // receives
  for (auto i = 0U;  i != total_send_count;  ++i)
  {
    TestFixture::socket.receive_async(TestFixture::service.make_io());
  }

  std::atomic<size_t> sends{}, receives{};
  for (auto &thread: threads)
  {
    thread = std::thread([&]
    {
      // sends
      for (auto i = 0U;  i < per_thread_send_count;  ++i)
      {
        TestFixture::socket.send_async(TestFixture::service.make_io());
      }

      // completions
      const auto deadline = std::chrono::steady_clock::now() + 250ms;
      while (std::chrono::steady_clock::now() < deadline)
      {
        std::error_code error;
        if (auto io = TestFixture::service.poll(10ms, error))
        {
          EXPECT_TRUE(!error) << "poll: " << error.message();

          if (io.template get_if<socket_t::send_t>(error))
          {
            sends++;
          }
          else if (io.template get_if<socket_t::receive_t>(error))
          {
            receives++;
          }
          else
          {
            FAIL() << "unexpected completion";
          }
          EXPECT_TRUE(!error) << "completion: " << error.message();

          if (sends == total_send_count && receives == sends)
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

  TestFixture::socket.close();

  // must send everything
  EXPECT_EQ(total_send_count, sends);

  // but can miss any number receives depending on IP stack impl
  size_t canceled = 0;
  std::error_code error;
  while (auto io = TestFixture::service.try_poll())
  {
    EXPECT_TRUE(io.template get_if<socket_t::receive_t>(error));
    EXPECT_EQ(std::errc::operation_canceled, error);
    canceled++;
  }
  EXPECT_EQ(total_send_count - canceled, receives);
}


//}}}1


} // namespace
