#include <sal/net/ip/tcp.hpp>
#include <sal/common.test.hpp>
#include <thread>


namespace {


using namespace std::chrono_literals;


struct socket_acceptor
  : public sal_test::with_value<sal::net::ip::tcp_t>
{
  using socket_t = sal::net::ip::tcp_t::socket_t;
  using acceptor_t = sal::net::ip::tcp_t::acceptor_t;

  const sal::net::ip::tcp_t protocol = GetParam();

  const socket_t::endpoint_t endpoint =
    protocol == sal::net::ip::tcp_t::v4
      ? socket_t::endpoint_t{sal::net::ip::address_v4_t::loopback, 8195}
      : socket_t::endpoint_t{sal::net::ip::address_v6_t::loopback, 8195}
  ;
};


INSTANTIATE_TEST_CASE_P(net_ip, socket_acceptor,
  ::testing::Values(
    sal::net::ip::tcp_t::v4,
    sal::net::ip::tcp_t::v6
  ),
  ::testing::PrintToStringParamName()
);


TEST_P(socket_acceptor, ctor)
{
  acceptor_t acceptor;
  EXPECT_FALSE(acceptor.is_open());
  EXPECT_FALSE(acceptor.enable_connection_aborted());
}


TEST_P(socket_acceptor, ctor_move)
{
  acceptor_t a(protocol);
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
  acceptor_t acceptor(protocol);
  EXPECT_TRUE(acceptor.is_open());
}


TEST_P(socket_acceptor, ctor_handle)
{
  auto handle = sal::net::socket_base_t::invalid - 1;
  acceptor_t acceptor(protocol, handle);
  EXPECT_EQ(handle, acceptor.native_handle());

  std::error_code ignored;
  acceptor.close(ignored);
}


TEST_P(socket_acceptor, ctor_endpoint)
{
  acceptor_t acceptor(endpoint);
  EXPECT_TRUE(acceptor.is_open());
  EXPECT_FALSE(acceptor.enable_connection_aborted());

  bool reuse_address;
  acceptor.get_option(sal::net::reuse_address(&reuse_address));
  EXPECT_TRUE(reuse_address);
}


TEST_P(socket_acceptor, ctor_address_already_in_use)
{
  acceptor_t a(endpoint);
  EXPECT_THROW(acceptor_t b(endpoint, false), std::system_error);
}


TEST_P(socket_acceptor, ctor_invalid_handle)
{
  auto h = sal::net::socket_base_t::invalid;
  EXPECT_THROW(acceptor_t(protocol, h), std::system_error);
}


TEST_P(socket_acceptor, assign_move)
{
  acceptor_t a(protocol), b;
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

  auto h = sal::net::socket_base_t::invalid - 1;
  acceptor.assign(protocol, h);
  EXPECT_TRUE(acceptor.is_open());
  EXPECT_EQ(h, acceptor.native_handle());

  std::error_code ignored;
  acceptor.close(ignored);
}


TEST_P(socket_acceptor, assign_not_closed)
{
  acceptor_t acceptor(protocol);
  auto h = sal::net::socket_base_t::invalid - 1;

  {
    std::error_code error;
    acceptor.assign(protocol, h, error);
    EXPECT_EQ(sal::net::socket_errc::already_open, error);
  }

  {
    EXPECT_THROW(acceptor.assign(protocol, h), std::system_error);
  }
}


TEST_P(socket_acceptor, assign_invalid_handle)
{
  acceptor_t acceptor;
  auto h = sal::net::socket_base_t::invalid;

  {
    std::error_code error;
    acceptor.assign(protocol, h, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(acceptor.assign(protocol, h), std::system_error);
  }
}


TEST_P(socket_acceptor, open)
{
  acceptor_t acceptor;
  acceptor.open(protocol);
  EXPECT_TRUE(acceptor.is_open());
}


TEST_P(socket_acceptor, open_already_open)
{
  acceptor_t acceptor(protocol);

  {
    std::error_code error;
    acceptor.open(protocol, error);
    EXPECT_EQ(sal::net::socket_errc::already_open, error);
    EXPECT_TRUE(acceptor.is_open());
  }

  {
    EXPECT_THROW(acceptor.open(protocol), std::system_error);
    EXPECT_TRUE(acceptor.is_open());
  }
}


TEST_P(socket_acceptor, close)
{
  acceptor_t acceptor;

  acceptor.open(protocol);
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
  acceptor_t acceptor(protocol, sal::net::socket_base_t::invalid - 1);

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
  acceptor_t acceptor(protocol);

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
  acceptor_t acceptor(protocol);
  acceptor.set_option(sal::net::reuse_address(true));
  acceptor.bind(endpoint);
  EXPECT_EQ(endpoint, acceptor.local_endpoint());
}


TEST_P(socket_acceptor, bind_invalid)
{
  acceptor_t acceptor;

  {
    std::error_code error;
    acceptor.bind(endpoint, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      acceptor.bind(endpoint),
      std::system_error
    );
  }
}


TEST_P(socket_acceptor, listen)
{
  acceptor_t acceptor(protocol);
  acceptor.set_option(sal::net::reuse_address(true));
  acceptor.bind(endpoint);
  acceptor.listen();

  socket_t a;
  a.connect(endpoint);

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
  acceptor_t acceptor(endpoint, true);

  socket_t a;
  a.connect(endpoint);

  socket_t::endpoint_t remote_endpoint;
  auto b = acceptor.accept(remote_endpoint);

  EXPECT_EQ(remote_endpoint, b.remote_endpoint());
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
  acceptor_t::endpoint_t remote_endpoint;

  {
    std::error_code error;
    acceptor.accept(remote_endpoint, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(acceptor.accept(remote_endpoint), std::system_error);
  }
}


TEST_P(socket_acceptor, wait)
{
  acceptor_t acceptor(endpoint, true);
  EXPECT_FALSE(acceptor.wait(acceptor.wait_read, 0s));

  socket_t a;
  a.connect(endpoint);
  EXPECT_TRUE(acceptor.wait(acceptor.wait_read, 10s));
  acceptor.accept();
}


TEST_P(socket_acceptor, enable_connection_aborted)
{
  acceptor_t acceptor(endpoint, true);
  acceptor.enable_connection_aborted(true);
  EXPECT_TRUE(acceptor.enable_connection_aborted());

  // Theoretically this should generate ECONNABORTED
  // Unfortunately it is platform dependent

  /*
  socket_t socket;
  socket.connect(endpoint);

  socket.set_option(sal::net::linger(true, 0s));

  socket_t::endpoint_t endpoint;
  std::error_code error;
  acceptor.accept(endpoint, error);
  EXPECT_EQ(std::errc::connection_aborted, error);
  */
}


TEST_P(socket_acceptor, local_endpoint)
{
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


} // namespace
