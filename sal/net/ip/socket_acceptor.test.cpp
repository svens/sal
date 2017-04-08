#include <sal/net/ip/tcp.hpp>
#include <sal/net/io_context.hpp>
#include <sal/net/io_service.hpp>
#include <sal/common.test.hpp>
#include <thread>


namespace {


struct socket_acceptor
  : public sal_test::with_value<sal::net::ip::tcp_t>
{
  using socket_t = sal::net::ip::tcp_t::socket_t;
  using acceptor_t = sal::net::ip::tcp_t::acceptor_t;
  static constexpr sal::net::ip::port_t port = 8193;

  acceptor_t::endpoint_t loopback (const sal::net::ip::tcp_t &protocol) const
  {
    return protocol == sal::net::ip::tcp_t::v4()
      ? acceptor_t::endpoint_t(sal::net::ip::address_v4_t::loopback(), port)
      : acceptor_t::endpoint_t(sal::net::ip::address_v6_t::loopback(), port)
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

constexpr sal::net::ip::port_t socket_acceptor::port;


INSTANTIATE_TEST_CASE_P(net_ip, socket_acceptor,
  ::testing::Values(
    sal::net::ip::tcp_t::v4(),
    sal::net::ip::tcp_t::v6()
  )
);


TEST_P(socket_acceptor, ctor)
{
  acceptor_t acceptor;
  EXPECT_FALSE(acceptor.is_open());
  EXPECT_FALSE(acceptor.enable_connection_aborted());
}


TEST_P(socket_acceptor, ctor_move)
{
  acceptor_t a(GetParam());
  EXPECT_TRUE(a.is_open());
  auto b = std::move(a);
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TEST_P(socket_acceptor, ctor_move_invalid_handle)
{
  acceptor_t a;
  EXPECT_FALSE(a.is_open());
  auto b{std::move(a)};
  EXPECT_FALSE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TEST_P(socket_acceptor, ctor_protocol)
{
  acceptor_t acceptor(GetParam());
  EXPECT_TRUE(acceptor.is_open());
}


TEST_P(socket_acceptor, ctor_protocol_and_handle)
{
  auto handle = sal::net::socket_base_t::invalid_socket - 1;
  acceptor_t acceptor(GetParam(), handle);
  EXPECT_EQ(handle, acceptor.native_handle());

  std::error_code ignored;
  acceptor.close(ignored);
}


TEST_P(socket_acceptor, ctor_endpoint)
{
  acceptor_t::endpoint_t endpoint(GetParam(), port);
  acceptor_t acceptor(endpoint);
  EXPECT_TRUE(acceptor.is_open());
  EXPECT_FALSE(acceptor.enable_connection_aborted());

  endpoint = acceptor.local_endpoint();
  EXPECT_TRUE(endpoint.address().is_unspecified());
  EXPECT_EQ(port, endpoint.port());

  bool reuse_address;
  acceptor.get_option(sal::net::reuse_address(&reuse_address));
  EXPECT_TRUE(reuse_address);
}


TEST_P(socket_acceptor, ctor_address_already_in_use)
{
  acceptor_t::endpoint_t endpoint(GetParam(), port);
  acceptor_t a(endpoint);
  EXPECT_THROW(acceptor_t b(endpoint, false), std::system_error);
}


TEST_P(socket_acceptor, ctor_invalid_handle)
{
  auto h = sal::net::socket_base_t::invalid_socket;
  EXPECT_THROW(acceptor_t(GetParam(), h), std::system_error);
}


TEST_P(socket_acceptor, assign_move)
{
  acceptor_t a(GetParam()), b;
  EXPECT_TRUE(a.is_open());
  EXPECT_FALSE(b.is_open());

  auto handle = a.native_handle();
  b = std::move(a);
  EXPECT_EQ(handle, b.native_handle());
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TEST_P(socket_acceptor, assign)
{
  acceptor_t acceptor;

  auto h = sal::net::socket_base_t::invalid_socket - 1;
  acceptor.assign(GetParam(), h);
  EXPECT_TRUE(acceptor.is_open());
  EXPECT_EQ(h, acceptor.native_handle());

  std::error_code ignored;
  acceptor.close(ignored);
}


TEST_P(socket_acceptor, assign_not_closed)
{
  acceptor_t acceptor(GetParam());
  auto h = sal::net::socket_base_t::invalid_socket - 1;

  {
    std::error_code error;
    acceptor.assign(GetParam(), h, error);
    EXPECT_EQ(sal::net::socket_errc_t::already_open, error);
  }

  {
    EXPECT_THROW(acceptor.assign(GetParam(), h), std::system_error);
  }
}


TEST_P(socket_acceptor, assign_invalid_handle)
{
  acceptor_t acceptor;
  auto h = sal::net::socket_base_t::invalid_socket;

  {
    std::error_code error;
    acceptor.assign(GetParam(), h, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(acceptor.assign(GetParam(), h), std::system_error);
  }
}


TEST_P(socket_acceptor, open)
{
  acceptor_t acceptor;
  acceptor.open(GetParam());
  EXPECT_TRUE(acceptor.is_open());
}


TEST_P(socket_acceptor, open_already_open)
{
  acceptor_t acceptor(GetParam());

  {
    std::error_code error;
    acceptor.open(GetParam(), error);
    EXPECT_EQ(sal::net::socket_errc_t::already_open, error);
    EXPECT_TRUE(acceptor.is_open());
  }

  {
    EXPECT_THROW(acceptor.open(GetParam()), std::system_error);
    EXPECT_TRUE(acceptor.is_open());
  }
}


TEST_P(socket_acceptor, close)
{
  acceptor_t acceptor;

  acceptor.open(GetParam());
  EXPECT_TRUE(acceptor.is_open());

  acceptor.close();
  EXPECT_FALSE(acceptor.is_open());
}


TEST_P(socket_acceptor, close_invalid_handle)
{
  acceptor_t acceptor;

  {
    std::error_code error;
    acceptor.close(error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(acceptor.close(), std::system_error);
  }
}


TEST_P(socket_acceptor, close_bad_file_descriptor)
{
  acceptor_t acceptor(GetParam(), sal::net::socket_base_t::invalid_socket - 1);

  {
    std::error_code error;
    acceptor.close(error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(acceptor.close(), std::system_error);
  }
}


TEST_P(socket_acceptor, non_blocking)
{
  acceptor_t acceptor(GetParam());

#if __sal_os_windows

  // no way to query this setting on Windows
  acceptor.non_blocking(false);
  acceptor.non_blocking(true);

#else

  bool non_blocking = acceptor.non_blocking();
  acceptor.non_blocking(!non_blocking);
  EXPECT_NE(non_blocking, acceptor.non_blocking());
  acceptor.non_blocking(non_blocking);
  EXPECT_EQ(non_blocking, acceptor.non_blocking());

#endif
}


TEST_P(socket_acceptor, non_blocking_invalid)
{
  acceptor_t acceptor;

  {
    std::error_code error;
    acceptor.non_blocking(true, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      acceptor.non_blocking(true),
      std::system_error
    );
  }

  {
    std::error_code error;
    (void)acceptor.non_blocking(error);
#if __sal_os_windows
    EXPECT_EQ(std::errc::operation_not_supported, error);
#else
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
#endif
  }

  {
    EXPECT_THROW(
      acceptor.non_blocking(),
      std::system_error
    );
  }
}


TEST_P(socket_acceptor, bind)
{
  acceptor_t acceptor(GetParam());
  acceptor.set_option(sal::net::reuse_address(true));
  acceptor.bind(loopback(GetParam()));
  EXPECT_EQ(loopback(GetParam()), acceptor.local_endpoint());
}


TEST_P(socket_acceptor, bind_invalid)
{
  acceptor_t acceptor;

  {
    std::error_code error;
    acceptor.bind(loopback(GetParam()), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      acceptor.bind(loopback(GetParam())),
      std::system_error
    );
  }
}


TEST_P(socket_acceptor, listen)
{
  acceptor_t acceptor(GetParam());
  acceptor.set_option(sal::net::reuse_address(true));
  acceptor.bind(loopback(GetParam()));
  acceptor.listen();

  socket_t a;
  a.connect(loopback(GetParam()));

  socket_t::endpoint_t endpoint;
  auto b = acceptor.accept();
  EXPECT_EQ(a.remote_endpoint(), b.local_endpoint());
  EXPECT_EQ(b.remote_endpoint(), a.local_endpoint());
}


TEST_P(socket_acceptor, listen_invalid)
{
  acceptor_t acceptor;

  {
    std::error_code error;
    acceptor.listen(3, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(acceptor.listen(3), std::system_error);
  }
}


TEST_P(socket_acceptor, accept)
{
  acceptor_t acceptor(loopback(GetParam()), true);

  socket_t a;
  a.connect(loopback(GetParam()));

  socket_t::endpoint_t endpoint;
  auto b = acceptor.accept(endpoint);

  EXPECT_EQ(endpoint, b.remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), b.local_endpoint());
  EXPECT_EQ(b.remote_endpoint(), a.local_endpoint());
}


TEST_P(socket_acceptor, accept_with_invalid_socket)
{
  acceptor_t acceptor;

  {
    std::error_code error;
    acceptor.accept(error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(acceptor.accept(), std::system_error);
  }
}


TEST_P(socket_acceptor, accept_with_invalid_socket_and_endpoint)
{
  acceptor_t acceptor;
  acceptor_t::endpoint_t endpoint;

  {
    std::error_code error;
    acceptor.accept(endpoint, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(acceptor.accept(endpoint), std::system_error);
  }
}


TEST_P(socket_acceptor, wait)
{
  using namespace std::chrono_literals;

  acceptor_t acceptor(loopback(GetParam()), true);
  EXPECT_FALSE(acceptor.wait(acceptor.wait_read, 0s));

  socket_t a;
  a.connect(loopback(GetParam()));
  EXPECT_TRUE(acceptor.wait(acceptor.wait_read, 10s));
  acceptor.accept();
}


TEST_P(socket_acceptor, enable_connection_aborted)
{
  acceptor_t acceptor(loopback(GetParam()), true);
  acceptor.enable_connection_aborted(true);
  EXPECT_TRUE(acceptor.enable_connection_aborted());

  // Theoretically this should generate ECONNABORTED
  // Unfortunately it is platform dependent

  /*
  socket_t socket;
  socket.connect(loopback(GetParam()));

  using namespace std::chrono_literals;
  socket.set_option(sal::net::linger(true, 0s));

  socket_t::endpoint_t endpoint;
  std::error_code error;
  acceptor.accept(endpoint, error);
  EXPECT_EQ(std::errc::connection_aborted, error);
  */
}


TEST_P(socket_acceptor, local_endpoint)
{
  auto endpoint = loopback(GetParam());
  acceptor_t acceptor(endpoint);
  EXPECT_EQ(endpoint, acceptor.local_endpoint());
}


TEST_P(socket_acceptor, local_endpoint_invalid)
{
  acceptor_t acceptor;

  {
    std::error_code error;
    acceptor.local_endpoint(error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(acceptor.local_endpoint(), std::system_error);
  }
}


TEST_P(socket_acceptor, async_accept)
{
  acceptor_t acceptor(loopback(GetParam()), true);
  acceptor.non_blocking(true);
  service.associate(acceptor);

  acceptor.async_accept(context.make_buf());

  socket_t a;
  a.connect(loopback(GetParam()));

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  auto result = acceptor.async_accept_result(io_buf);
  ASSERT_NE(nullptr, result);

  EXPECT_EQ(a.local_endpoint(), result->remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), result->local_endpoint());

  auto b = result->accepted();
  EXPECT_EQ(a.local_endpoint(), b.remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), b.local_endpoint());

  EXPECT_EQ(nullptr, a.async_connect_result(io_buf));
}


TEST_P(socket_acceptor, async_accept_immediate_completion)
{
  acceptor_t acceptor(loopback(GetParam()), true);
  acceptor.non_blocking(true);
  service.associate(acceptor);

  socket_t a;
  a.connect(loopback(GetParam()));

  acceptor.async_accept(context.make_buf());

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  auto result = acceptor.async_accept_result(io_buf);
  ASSERT_NE(nullptr, result);

  EXPECT_EQ(a.local_endpoint(), result->remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), result->local_endpoint());

  auto b = result->accepted();
  EXPECT_EQ(a.local_endpoint(), b.remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), b.local_endpoint());

  EXPECT_EQ(nullptr, a.async_connect_result(io_buf));
}


TEST_P(socket_acceptor, async_accept_result_twice)
{
  acceptor_t acceptor(loopback(GetParam()), true);
  acceptor.non_blocking(true);
  service.associate(acceptor);

  socket_t a;
  a.connect(loopback(GetParam()));
  acceptor.async_accept(context.make_buf());

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  auto result1 = acceptor.async_accept_result(io_buf);
  ASSERT_NE(nullptr, result1);

  auto result2 = acceptor.async_accept_result(io_buf);
  ASSERT_NE(nullptr, result2);

  EXPECT_EQ(result1, result2);

  EXPECT_EQ(a.local_endpoint(), result1->remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), result1->local_endpoint());

  auto b = result2->accepted();
  EXPECT_EQ(a.local_endpoint(), b.remote_endpoint());
  EXPECT_EQ(a.remote_endpoint(), b.local_endpoint());
}


TEST_P(socket_acceptor, async_accept_invalid)
{
  acceptor_t acceptor(loopback(GetParam()), true);
  acceptor.non_blocking(true);
  service.associate(acceptor);
  acceptor.close();

  acceptor.async_accept(context.make_buf());

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  {
    std::error_code error;
    auto result = acceptor.async_accept_result(io_buf, error);
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      acceptor.async_accept_result(io_buf),
      std::system_error
    );
  }
}


TEST_P(socket_acceptor, async_accept_close_before_accept)
{
  acceptor_t acceptor(loopback(GetParam()), true);
  acceptor.non_blocking(true);
  service.associate(acceptor);

  socket_t a;
  a.connect(loopback(GetParam()));
  a.close();
  std::this_thread::yield();

  acceptor.async_accept(context.make_buf());

  auto io_buf = context.get();
  ASSERT_NE(nullptr, io_buf);

  // accept succeeds
  std::error_code error;
  auto result = acceptor.async_accept_result(io_buf, error);
  ASSERT_NE(nullptr, result);
  EXPECT_FALSE(error);

  // but receive should fail
  auto b = result->accepted();
  char buf[1024];
  b.receive(sal::make_buf(buf), error);
  EXPECT_EQ(sal::net::socket_errc_t::orderly_shutdown, error);
}


} // namespace
