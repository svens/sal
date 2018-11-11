#include <sal/net/async/service.hpp>
#include <sal/net/ip/tcp.hpp>
#include <sal/net/common.test.hpp>
#include <thread>


namespace {


using socket_t = sal::net::ip::tcp_t::socket_t;
using acceptor_t = sal::net::ip::tcp_t::acceptor_t;


template <typename Address>
struct net_async_socket_acceptor
  : public sal_test::with_type<Address>
{
  const sal::net::ip::tcp_t protocol =
    std::is_same_v<Address, sal::net::ip::address_v4_t>
      ? sal::net::ip::tcp_t::v4
      : sal::net::ip::tcp_t::v6
  ;

  const sal::net::ip::tcp_t::endpoint_t endpoint{
    Address::loopback,
    8195
  };

  sal::net::async::service_t service{};
  acceptor_t acceptor{endpoint};


  void SetUp ()
  {
    acceptor.non_blocking(true);
    acceptor.associate(service);
  }


  sal::net::async::io_ptr wait ()
  {
    auto io = service.try_get();
    if (!io && service.wait())
    {
      io = service.try_get();
    }
    return io;
  }
};

TYPED_TEST_CASE(net_async_socket_acceptor,
  sal_test::address_types,
  sal_test::address_names
);


TYPED_TEST(net_async_socket_acceptor, start_accept) //{{{1
{
  TestFixture::acceptor.start_accept(TestFixture::service.make_io());

  socket_t a;
  a.connect(TestFixture::endpoint);

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  EXPECT_EQ(nullptr, io->template get_if<socket_t::connect_t>());

  auto result = io->template get_if<acceptor_t::accept_t>();
  ASSERT_NE(nullptr, result);

  auto b = result->accepted_socket();
  EXPECT_TRUE(b.is_open());
  EXPECT_EQ(a.local_endpoint(), b.remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), b.local_endpoint());
}


TYPED_TEST(net_async_socket_acceptor, start_accept_with_context) //{{{1
{
  int socket_ctx = 1, io_ctx = 2;
  TestFixture::acceptor.context(&socket_ctx);

  TestFixture::acceptor.start_accept(TestFixture::service.make_io(&io_ctx));

  socket_t a;
  a.connect(TestFixture::endpoint);

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  EXPECT_EQ(&io_ctx, io->template context<int>());
  EXPECT_EQ(nullptr, io->template context<socket_t>());
  EXPECT_EQ(&socket_ctx, io->template socket_context<int>());
  EXPECT_EQ(nullptr, io->template socket_context<socket_t>());

  auto result = io->template get_if<acceptor_t::accept_t>();
  ASSERT_NE(nullptr, result);

  auto b = result->accepted_socket();
  EXPECT_TRUE(b.is_open());
  EXPECT_EQ(a.local_endpoint(), b.remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), b.local_endpoint());
}


TYPED_TEST(net_async_socket_acceptor, start_accept_immediate_completion) //{{{1
{
  socket_t a;
  a.connect(TestFixture::endpoint);

  TestFixture::acceptor.start_accept(TestFixture::service.make_io());

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  auto result = io->template get_if<acceptor_t::accept_t>();
  ASSERT_NE(nullptr, result);

  auto b = result->accepted_socket();
  EXPECT_TRUE(b.is_open());
  EXPECT_EQ(a.local_endpoint(), b.remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), b.local_endpoint());
}


TYPED_TEST(net_async_socket_acceptor, start_accept_result_multiple_times) //{{{1
{
  TestFixture::acceptor.start_accept(TestFixture::service.make_io());

  socket_t a;
  a.connect(TestFixture::endpoint);

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  auto result = io->template get_if<acceptor_t::accept_t>();
  ASSERT_NE(nullptr, result);

  auto b = result->accepted_socket();
  EXPECT_TRUE(b.is_open());

  std::error_code error;
  auto c = result->accepted_socket(error);
  EXPECT_EQ(std::errc::bad_file_descriptor, error);
  EXPECT_FALSE(c.is_open());

  EXPECT_THROW(auto d = result->accepted_socket(), std::system_error);
}


TYPED_TEST(net_async_socket_acceptor, start_accept_and_close) //{{{1
{
  TestFixture::acceptor.start_accept(TestFixture::service.make_io());
  TestFixture::acceptor.close();

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  std::error_code error;
  auto result = io->template get_if<acceptor_t::accept_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(std::errc::operation_canceled, error);
}


TYPED_TEST(net_async_socket_acceptor, start_accept_close_before_accept) //{{{1
{
  socket_t a;
  a.connect(TestFixture::endpoint);
  a.close();
  std::this_thread::yield();

  TestFixture::acceptor.start_accept(TestFixture::service.make_io());

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  // accept succeeds
  std::error_code error;
  auto result = io->template get_if<acceptor_t::accept_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_FALSE(error);

  // but receive should fail
  auto b = result->accepted_socket();
  char buf[1024];
  b.receive(buf, error);
  EXPECT_EQ(std::errc::broken_pipe, error);
}


TYPED_TEST(net_async_socket_acceptor, start_accept_close_after_accept) //{{{1
{
  TestFixture::acceptor.start_accept(TestFixture::service.make_io());

  socket_t a;
  a.connect(TestFixture::endpoint);
  a.close();
  std::this_thread::yield();

  auto io = TestFixture::wait();
  ASSERT_NE(nullptr, io);

  // accept succeeds
  std::error_code error;
  auto result = io->template get_if<acceptor_t::accept_t>(error);
  ASSERT_NE(nullptr, result);
  EXPECT_FALSE(error);

  // but receive should fail
  auto b = result->accepted_socket();
  char buf[1024];
  b.receive(buf, error);
  EXPECT_EQ(std::errc::broken_pipe, error);
}


//}}}1


} // namespace
