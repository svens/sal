#include <sal/net/basic_socket.hpp>
#include <sal/net/ip/tcp.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/net/common.test.hpp>


namespace {


template <typename ProtocolAndAddress>
struct net_socket
  : public sal_test::with_type<ProtocolAndAddress>
{
  using protocol_t = typename ProtocolAndAddress::first_type;
  using address_t = typename ProtocolAndAddress::second_type;
  using socket_t = typename protocol_t::socket_t;
  using endpoint_t = typename protocol_t::endpoint_t;

  const protocol_t protocol =
    std::is_same_v<address_t, sal::net::ip::address_v4_t>
      ? protocol_t::v4
      : protocol_t::v6
  ;

  static constexpr sal::net::socket_base_t::handle_t handle =
    sal::net::socket_base_t::invalid - 1;
};

TYPED_TEST_CASE(net_socket,
  sal_test::protocol_and_address_types,
  sal_test::protocol_and_address_names
);


TYPED_TEST(net_socket, ctor)
{
  typename TestFixture::socket_t socket;
  EXPECT_FALSE(socket.is_open());
}


TYPED_TEST(net_socket, ctor_with_protocol)
{
  typename TestFixture::socket_t a(TestFixture::protocol);
  EXPECT_TRUE(a.is_open());
}


TYPED_TEST(net_socket, ctor_move)
{
  typename TestFixture::socket_t a(TestFixture::protocol);
  EXPECT_TRUE(a.is_open());
  auto b{std::move(a)};
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TYPED_TEST(net_socket, ctor_move_no_handle)
{
  typename TestFixture::socket_t a;
  EXPECT_FALSE(a.is_open());
  auto b{std::move(a)};
  EXPECT_FALSE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TYPED_TEST(net_socket, ctor_with_handle)
{
  typename TestFixture::socket_t socket(TestFixture::handle);
  EXPECT_EQ(TestFixture::handle, socket.native_handle());
}


TYPED_TEST(net_socket, ctor_with_endpoint)
{
  typename TestFixture::endpoint_t endpoint(TestFixture::protocol, 0);
  typename TestFixture::socket_t socket(endpoint);

  endpoint = socket.local_endpoint();
  EXPECT_TRUE(endpoint.address().is_unspecified());
  EXPECT_NE(0U, endpoint.port());
}


TYPED_TEST(net_socket, assign_move)
{
  typename TestFixture::socket_t a(TestFixture::protocol), b;
  EXPECT_TRUE(a.is_open());
  EXPECT_FALSE(b.is_open());

  auto h = a.native_handle();
  b = std::move(a);
  EXPECT_EQ(h, b.native_handle());
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TYPED_TEST(net_socket, open)
{
  typename TestFixture::socket_t socket;
  socket.open(TestFixture::protocol);
  EXPECT_TRUE(socket.is_open());
}


TYPED_TEST(net_socket, open_already_open)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);
  EXPECT_TRUE(socket.is_open());

  {
    std::error_code error;
    socket.open(TestFixture::protocol, error);
    EXPECT_EQ(sal::net::socket_errc::already_open, error);
    EXPECT_TRUE(socket.is_open());
  }

  {
    EXPECT_THROW(socket.open(TestFixture::protocol), std::system_error);
    EXPECT_TRUE(socket.is_open());
  }
}


TYPED_TEST(net_socket, assign)
{
  typename TestFixture::socket_t socket;

  socket.assign(TestFixture::handle);
  EXPECT_TRUE(socket.is_open());
  EXPECT_EQ(TestFixture::handle, socket.native_handle());

  std::error_code ignored;
  socket.close(ignored);
}


TYPED_TEST(net_socket, assign_not_closed)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);

  {
    std::error_code error;
    socket.assign(TestFixture::handle, error);
    EXPECT_EQ(sal::net::socket_errc::already_open, error);
  }

  {
    EXPECT_THROW(socket.assign(TestFixture::handle), std::system_error);
  }
}


TYPED_TEST(net_socket, assign_invalid)
{
  typename TestFixture::socket_t socket;

  {
    std::error_code error;
    socket.assign(sal::net::socket_base_t::invalid, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.assign(sal::net::socket_base_t::invalid),
      std::system_error
    );
  }
}



TYPED_TEST(net_socket, close)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);
  EXPECT_TRUE(socket.is_open());

  socket.close();
  EXPECT_FALSE(socket.is_open());
}


TYPED_TEST(net_socket, close_no_handle)
{
  typename TestFixture::socket_t socket;

  {
    std::error_code error;
    socket.close(error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.close(), std::system_error);
  }
}


TYPED_TEST(net_socket, close_bad_file_descriptor)
{
  typename TestFixture::socket_t socket(TestFixture::handle);

  {
    std::error_code error;
    socket.close(error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.close(), std::system_error);
  }
}


TYPED_TEST(net_socket, broadcast)
{
  if constexpr (std::is_same_v<typename TestFixture::protocol_t, sal::net::ip::udp_t>)
  {
    typename TestFixture::socket_t socket(TestFixture::protocol);

    bool original, value;
    socket.get_option(sal::net::broadcast(&original));
    socket.set_option(sal::net::broadcast(!original));
    socket.get_option(sal::net::broadcast(&value));
    EXPECT_NE(original, value);
  }
}


TYPED_TEST(net_socket, broadcast_invalid)
{
  typename TestFixture::socket_t socket;
  bool value{false};

  {
    std::error_code error;
    socket.get_option(sal::net::broadcast(&value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.get_option(sal::net::broadcast(&value)),
      std::system_error
    );
  }

  {
    std::error_code error;
    socket.set_option(sal::net::broadcast(value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.set_option(sal::net::broadcast(value)),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, debug)
{
#if __sal_os_linux

  // requires CAP_NET_ADMIN or root

#else

  typename TestFixture::socket_t socket(TestFixture::protocol);

  bool original, value;
  socket.get_option(sal::net::debug(&original));
  socket.set_option(sal::net::debug(!original));
  socket.get_option(sal::net::debug(&value));
  EXPECT_NE(original, value);

#endif
}


TYPED_TEST(net_socket, debug_invalid)
{
  typename TestFixture::socket_t socket;
  bool value{false};

  {
    std::error_code error;
    socket.get_option(sal::net::debug(&value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.get_option(sal::net::debug(&value)),
      std::system_error
    );
  }

  {
    std::error_code error;
    socket.set_option(sal::net::debug(value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.set_option(sal::net::debug(value)),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, do_not_route)
{
#if __sal_os_windows

  // MS providers don't support this option

#else

  typename TestFixture::socket_t socket(TestFixture::protocol);

  bool original, value;
  socket.get_option(sal::net::do_not_route(&original));
  socket.set_option(sal::net::do_not_route(!original));
  socket.get_option(sal::net::do_not_route(&value));
  EXPECT_NE(original, value);

#endif
}


TYPED_TEST(net_socket, do_not_route_invalid)
{
  typename TestFixture::socket_t socket;
  bool value{false};

  {
    std::error_code error;
    socket.get_option(sal::net::do_not_route(&value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.get_option(sal::net::do_not_route(&value)),
      std::system_error
    );
  }

  {
    std::error_code error;
    socket.set_option(sal::net::do_not_route(value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.set_option(sal::net::do_not_route(value)),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, keep_alive)
{
  if constexpr (std::is_same_v<typename TestFixture::protocol_t, sal::net::ip::tcp_t>)
  {
    typename TestFixture::socket_t socket(TestFixture::protocol);

    bool original, value;
    socket.get_option(sal::net::keep_alive(&original));
    socket.set_option(sal::net::keep_alive(!original));
    socket.get_option(sal::net::keep_alive(&value));

    #if __sal_os_windows
      // Windows Vista and later can't change it
    #else
      EXPECT_NE(original, value);
    #endif
  }
}


TYPED_TEST(net_socket, keep_alive_invalid)
{
  typename TestFixture::socket_t socket;
  bool value{false};

  {
    std::error_code error;
    socket.get_option(sal::net::keep_alive(&value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.get_option(sal::net::keep_alive(&value)),
      std::system_error
    );
  }

  {
    std::error_code error;
    socket.set_option(sal::net::keep_alive(value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.set_option(sal::net::keep_alive(value)),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, reuse_address)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);

  bool original, value;
  socket.get_option(sal::net::reuse_address(&original));
  socket.set_option(sal::net::reuse_address(!original));
  socket.get_option(sal::net::reuse_address(&value));
  EXPECT_NE(original, value);
}


TYPED_TEST(net_socket, reuse_address_invalid)
{
  typename TestFixture::socket_t socket;
  bool value{false};

  {
    std::error_code error;
    socket.get_option(sal::net::reuse_address(&value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.get_option(sal::net::reuse_address(&value)),
      std::system_error
    );
  }

  {
    std::error_code error;
    socket.set_option(sal::net::reuse_address(value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.set_option(sal::net::reuse_address(value)),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, receive_buffer_size)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);

  int original, value;
  socket.get_option(sal::net::receive_buffer_size(&original));
  socket.set_option(sal::net::receive_buffer_size(2 * original));
  socket.get_option(sal::net::receive_buffer_size(&value));
  EXPECT_NE(original, value);
}


TYPED_TEST(net_socket, receive_buffer_size_invalid)
{
  typename TestFixture::socket_t socket;
  int value{0};

  {
    std::error_code error;
    socket.get_option(sal::net::receive_buffer_size(&value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.get_option(sal::net::receive_buffer_size(&value)),
      std::system_error
    );
  }

  {
    std::error_code error;
    socket.set_option(sal::net::receive_buffer_size(value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.set_option(sal::net::receive_buffer_size(value)),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, receive_low_watermark)
{
#if __sal_os_windows

  (void)protocol;

#else

  typename TestFixture::socket_t socket(TestFixture::protocol);

  int original, value;
  socket.get_option(sal::net::receive_low_watermark(&original));
  socket.set_option(sal::net::receive_low_watermark(2 * original));
  socket.get_option(sal::net::receive_low_watermark(&value));
  EXPECT_NE(original, value);

#endif
}


TYPED_TEST(net_socket, receive_low_watermark_invalid)
{
  typename TestFixture::socket_t socket;
  int value{0};

  {
    std::error_code error;
    socket.get_option(sal::net::receive_low_watermark(&value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.get_option(sal::net::receive_low_watermark(&value)),
      std::system_error
    );
  }

  {
    std::error_code error;
    socket.set_option(sal::net::receive_low_watermark(value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.set_option(sal::net::receive_low_watermark(value)),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, send_buffer_size)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);

  int original, value;
  socket.get_option(sal::net::send_buffer_size(&original));
  socket.set_option(sal::net::send_buffer_size(2 * original));
  socket.get_option(sal::net::send_buffer_size(&value));
  EXPECT_NE(original, value);
}


TYPED_TEST(net_socket, send_buffer_size_invalid)
{
  typename TestFixture::socket_t socket;
  int value{0};

  {
    std::error_code error;
    socket.get_option(sal::net::send_buffer_size(&value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.get_option(sal::net::send_buffer_size(&value)),
      std::system_error
    );
  }

  {
    std::error_code error;
    socket.set_option(sal::net::send_buffer_size(value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.set_option(sal::net::send_buffer_size(value)),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, send_low_watermark)
{
#if __sal_os_windows || __sal_os_linux

  // not changeable on those platforms

#else

  typename TestFixture::socket_t socket(TestFixture::protocol);

  int original, value;
  socket.get_option(sal::net::send_low_watermark(&original));
  socket.set_option(sal::net::send_low_watermark(2 * original));
  socket.get_option(sal::net::send_low_watermark(&value));
  EXPECT_NE(original, value);

#endif
}


TYPED_TEST(net_socket, send_low_watermark_invalid)
{
  typename TestFixture::socket_t socket;
  int value{0};

  {
    std::error_code error;
    socket.get_option(sal::net::send_low_watermark(&value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.get_option(sal::net::send_low_watermark(&value)),
      std::system_error
    );
  }

  {
    std::error_code error;
    socket.set_option(sal::net::send_low_watermark(value), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.set_option(sal::net::send_low_watermark(value)),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, linger)
{
  if constexpr (std::is_same_v<typename TestFixture::protocol_t, sal::net::ip::tcp_t>)
  {
    using namespace std::chrono_literals;

    typename TestFixture::socket_t socket(TestFixture::protocol);
    bool linger{};
    std::chrono::seconds timeout{};

    socket.get_option(sal::net::linger(&linger, &timeout));
    EXPECT_FALSE(linger);
    EXPECT_EQ(0s, timeout);

    socket.set_option(sal::net::linger(true, 3s));

    socket.get_option(sal::net::linger(&linger, &timeout));
    EXPECT_TRUE(linger);
    EXPECT_EQ(3s, timeout);
  }
}


TYPED_TEST(net_socket, linger_invalid)
{
  using namespace std::chrono_literals;

  typename TestFixture::socket_t socket;
  bool linger{};
  std::chrono::seconds timeout{};

  {
    std::error_code error;
    socket.get_option(sal::net::linger(&linger, &timeout), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.get_option(sal::net::linger(&linger, &timeout)),
      std::system_error
    );
  }

  {
    std::error_code error;
    socket.set_option(sal::net::linger(true, 3s), error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.set_option(sal::net::linger(true, 3s)),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, non_blocking)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);

#if __sal_os_windows

  // no way to query this setting on Windows
  socket.non_blocking(false);
  socket.non_blocking(true);

#else

  bool non_blocking = socket.non_blocking();
  socket.non_blocking(!non_blocking);
  EXPECT_NE(non_blocking, socket.non_blocking());
  socket.non_blocking(non_blocking);
  EXPECT_EQ(non_blocking, socket.non_blocking());

#endif
}


TYPED_TEST(net_socket, non_blocking_invalid)
{
  typename TestFixture::socket_t socket;

  {
    std::error_code error;
    socket.non_blocking(true, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.non_blocking(true),
      std::system_error
    );
  }

  {
    std::error_code error;
    (void)socket.non_blocking(error);
#if __sal_os_windows
    EXPECT_EQ(std::errc::operation_not_supported, error);
#else
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
#endif
  }

  {
    EXPECT_THROW(
      socket.non_blocking(),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, available)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);
  EXPECT_EQ(0U, socket.available());
}


TYPED_TEST(net_socket, available_invalid)
{
  typename TestFixture::socket_t socket;

  {
    std::error_code error;
    EXPECT_EQ(0U, socket.available(error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.available(),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, bind)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);
  typename TestFixture::endpoint_t endpoint(TestFixture::protocol, 0);
  socket.bind(endpoint);

  endpoint = socket.local_endpoint();
  EXPECT_TRUE(endpoint.address().is_unspecified());
  EXPECT_NE(0U, endpoint.port());
}


TYPED_TEST(net_socket, bind_invalid)
{
  typename TestFixture::socket_t socket;
  typename TestFixture::endpoint_t endpoint;

  {
    std::error_code error;
    socket.bind(endpoint, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.bind(endpoint),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, local_endpoint_invalid)
{
  typename TestFixture::socket_t socket;

  {
    std::error_code error;
    socket.local_endpoint(error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.local_endpoint(),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, local_endpoint_not_bound)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);
  std::error_code error;
  auto endpoint = socket.local_endpoint(error);

#if __sal_os_windows
  EXPECT_TRUE(bool(error));
  EXPECT_EQ(std::errc::invalid_argument, error);
#else
  EXPECT_FALSE(bool(error));
#endif

  EXPECT_TRUE(endpoint.address().is_unspecified());
  EXPECT_EQ(0U, endpoint.port());
}


TYPED_TEST(net_socket, remote_endpoint_invalid)
{
  typename TestFixture::socket_t socket;

  {
    std::error_code error;
    socket.remote_endpoint(error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.remote_endpoint(),
      std::system_error
    );
  }
}


TYPED_TEST(net_socket, remote_endpoint_not_connected)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);
  std::error_code error;
  auto endpoint = socket.remote_endpoint(error);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_TRUE(endpoint.address().is_unspecified());
  EXPECT_EQ(0U, endpoint.port());
}


TYPED_TEST(net_socket, connect_no_listener)
{
  typename TestFixture::socket_t socket(TestFixture::protocol);
  typename TestFixture::endpoint_t endpoint(TestFixture::address_t::loopback, 7);

  if constexpr (std::is_same_v<typename TestFixture::protocol_t, sal::net::ip::tcp_t>)
  {
    {
      std::error_code error;
      socket.connect(endpoint, error);
      EXPECT_EQ(std::errc::connection_refused, error);
    }

    {
      EXPECT_THROW(
        socket.connect(endpoint),
        std::system_error
      );
    }
  }
  else if constexpr (std::is_same_v<typename TestFixture::protocol_t, sal::net::ip::udp_t>)
  {
    {
      std::error_code error;
      socket.connect(endpoint, error);
      EXPECT_FALSE(bool(error));
      EXPECT_EQ(endpoint, socket.remote_endpoint());
    }

    {
      EXPECT_NO_THROW(socket.connect(endpoint));
    }
  }
}


TYPED_TEST(net_socket, shutdown)
{
  if constexpr (std::is_same_v<typename TestFixture::protocol_t, sal::net::ip::tcp_t>)
  {
    typename TestFixture::socket_t socket(TestFixture::protocol);
    auto what = socket.shutdown_both;

    {
      std::error_code error;
      socket.shutdown(what, error);
      EXPECT_EQ(std::errc::not_connected, error);
    }

    {
      EXPECT_THROW(socket.shutdown(what), std::system_error);
    }
  }
}


TYPED_TEST(net_socket, shutdown_invalid)
{
  typename TestFixture::socket_t socket;
  auto what = socket.shutdown_both;

  {
    std::error_code error;
    socket.shutdown(what, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.shutdown(what), std::system_error);
  }
}


TYPED_TEST(net_socket, wait)
{
  using namespace std::chrono_literals;

  typename TestFixture::socket_t socket(TestFixture::protocol);
  std::error_code error;

  if constexpr (std::is_same_v<typename TestFixture::protocol_t, sal::net::ip::tcp_t>)
  {
    {
      EXPECT_FALSE(socket.wait(socket.wait_write, 0ms, error));
      EXPECT_FALSE(bool(error));
    }

    {
      EXPECT_FALSE(socket.wait(socket.wait_read, 0ms, error));
      EXPECT_FALSE(bool(error));
    }
  }
  else
  {
    {
      EXPECT_TRUE(socket.wait(socket.wait_write, 0ms, error));
    }

    {
      EXPECT_FALSE(socket.wait(socket.wait_read, 0ms, error));
    }
  }
}


TYPED_TEST(net_socket, wait_invalid)
{
  using namespace std::chrono_literals;
  typename TestFixture::socket_t socket;
  std::error_code error;

  {
    EXPECT_FALSE(socket.wait(socket.wait_write, 0s, error));
    EXPECT_TRUE(bool(error));
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.wait(socket.wait_write, 0s), std::system_error);
  }
}


} // namespace
