#include <sal/net/basic_socket.hpp>
#include <sal/net/ip/tcp.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/common.test.hpp>


namespace {


template <typename Protocol>
struct net_socket
  : public sal_test::with_type<Protocol>
{};

using protocol_types = testing::Types<
  sal::net::ip::tcp_t,
  sal::net::ip::udp_t
>;
TYPED_TEST_CASE(net_socket, protocol_types);


template <typename Protocol,
  typename Socket=sal::net::basic_socket_t<Protocol>
>
struct socket_t
  : public Socket
{
  socket_t () = default;

  socket_t (const Protocol &protocol)
    : Socket(protocol)
  {}

  socket_t (const typename Protocol::endpoint_t &endpoint)
    : Socket(endpoint)
  {}

  socket_t (const Protocol &protocol,
      sal::net::socket_base_t::native_handle_t handle)
    : Socket(protocol, handle)
  {}
};


TYPED_TEST(net_socket, ctor)
{
  socket_t<TypeParam> socket;
  EXPECT_FALSE(socket.is_open());
}


template <typename Protocol>
void ctor_move (const Protocol &protocol)
{
  socket_t<Protocol> a(protocol);
  EXPECT_TRUE(a.is_open());
  auto b{std::move(a)};
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TYPED_TEST(net_socket, ctor_move_v4)
{
  ctor_move(TypeParam::v4());
}


TYPED_TEST(net_socket, ctor_move_v6)
{
  ctor_move(TypeParam::v6());
}


TYPED_TEST(net_socket, ctor_move_no_handle)
{
  socket_t<TypeParam> a;
  EXPECT_FALSE(a.is_open());
  auto b{std::move(a)};
  EXPECT_FALSE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TYPED_TEST(net_socket, ctor_protocol_v4)
{
  socket_t<TypeParam> socket(TypeParam::v4());
  EXPECT_TRUE(socket.is_open());
}


TYPED_TEST(net_socket, ctor_protocol_v6)
{
  socket_t<TypeParam> socket(TypeParam::v6());
  EXPECT_TRUE(socket.is_open());
}


template <typename Protocol>
void ctor_protocol_and_handle (const Protocol &protocol)
{
  auto handle = sal::net::socket_base_t::invalid_socket - 1;
  socket_t<Protocol> socket(protocol, handle);
  EXPECT_EQ(handle, socket.native_handle());

  std::error_code ignored;
  socket.close(ignored);
}


TYPED_TEST(net_socket, ctor_protocol_and_handle_v4)
{
  ctor_protocol_and_handle(TypeParam::v4());
}


TYPED_TEST(net_socket, ctor_protocol_and_handle_v6)
{
  ctor_protocol_and_handle(TypeParam::v6());
}


template <typename Protocol>
void ctor_endpoint (const Protocol &protocol)
{
  typename Protocol::endpoint_t endpoint(protocol, 0);
  socket_t<Protocol> socket(endpoint);

  endpoint = socket.local_endpoint();
  EXPECT_TRUE(endpoint.address().is_unspecified());
  EXPECT_NE(0U, endpoint.port());
}


TYPED_TEST(net_socket, ctor_endpoint_v4)
{
  ctor_endpoint(TypeParam::v4());
}


TYPED_TEST(net_socket, ctor_endpoint_v6)
{
  ctor_endpoint(TypeParam::v6());
}


template <typename Protocol>
void assign_move (const Protocol &protocol)
{
  socket_t<Protocol> a(protocol), b;
  EXPECT_TRUE(a.is_open());
  EXPECT_FALSE(b.is_open());

  auto handle = a.native_handle();
  b = std::move(a);
  EXPECT_EQ(handle, b.native_handle());
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TYPED_TEST(net_socket, assign_move_v4)
{
  assign_move(TypeParam::v4());
}


TYPED_TEST(net_socket, assign_move_v6)
{
  assign_move(TypeParam::v6());
}


TYPED_TEST(net_socket, open_v4)
{
  socket_t<TypeParam> socket;
  socket.open(TypeParam::v4());
  EXPECT_TRUE(socket.is_open());
}


TYPED_TEST(net_socket, open_v6)
{
  socket_t<TypeParam> socket;
  socket.open(TypeParam::v6());
  EXPECT_TRUE(socket.is_open());
}


template <typename Protocol>
void open_already_open (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);

  {
    std::error_code error;
    socket.open(protocol, error);
    EXPECT_EQ(sal::net::socket_errc_t::already_open, error);
    EXPECT_TRUE(socket.is_open());
  }

  {
    EXPECT_THROW(socket.open(protocol), std::system_error);
    EXPECT_TRUE(socket.is_open());
  }
}


TYPED_TEST(net_socket, open_already_open_v4)
{
  open_already_open(TypeParam::v4());
}


TYPED_TEST(net_socket, open_already_open_v6)
{
  open_already_open(TypeParam::v6());
}


template <typename Protocol>
void assign (const Protocol &protocol)
{
  socket_t<Protocol> socket;

  auto h = sal::net::socket_base_t::invalid_socket - 1;
  socket.assign(protocol, h);
  EXPECT_TRUE(socket.is_open());
  EXPECT_EQ(h, socket.native_handle());

  std::error_code ignored;
  socket.close(ignored);
}


TYPED_TEST(net_socket, assign_v4)
{
  assign(TypeParam::v4());
}


TYPED_TEST(net_socket, assign_v6)
{
  assign(TypeParam::v6());
}


template <typename Protocol>
void assign_not_closed (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);
  auto h = sal::net::socket_base_t::invalid_socket - 1;

  {
    std::error_code error;
    socket.assign(protocol, h, error);
    EXPECT_EQ(sal::net::socket_errc_t::already_open, error);
  }

  {
    EXPECT_THROW(socket.assign(protocol, h), std::system_error);
  }
}


TYPED_TEST(net_socket, assign_not_closed_v4)
{
  assign_not_closed(TypeParam::v4());
}


TYPED_TEST(net_socket, assign_not_closed_v6)
{
  assign_not_closed(TypeParam::v6());
}


template <typename Protocol>
void assign_no_handle (const Protocol &protocol)
{
  socket_t<Protocol> socket;
  auto h = sal::net::socket_base_t::invalid_socket;

  {
    std::error_code error;
    socket.assign(protocol, h, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.assign(protocol, h), std::system_error);
  }
}


TYPED_TEST(net_socket, assign_no_handle_v4)
{
  assign_no_handle(TypeParam::v4());
}


TYPED_TEST(net_socket, assign_no_handle_v6)
{
  assign_no_handle(TypeParam::v6());
}


template <typename Protocol>
void close (const Protocol &protocol)
{
  socket_t<Protocol> socket;

  socket.open(protocol);
  EXPECT_TRUE(socket.is_open());

  socket.close();
  EXPECT_FALSE(socket.is_open());
}


TYPED_TEST(net_socket, close_v4)
{
  close(TypeParam::v4());
}


TYPED_TEST(net_socket, close_v6)
{
  close(TypeParam::v6());
}


TYPED_TEST(net_socket, close_no_handle)
{
  socket_t<TypeParam> socket;

  {
    std::error_code error;
    socket.close(error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.close(), std::system_error);
  }
}


template <typename Protocol>
void close_bad_file_descriptor (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol,
    sal::net::socket_base_t::invalid_socket - 1
  );

  {
    std::error_code error;
    socket.close(error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.close(), std::system_error);
  }
}


TYPED_TEST(net_socket, close_bad_file_descriptor_v4)
{
  close_bad_file_descriptor(TypeParam::v4());
}


TYPED_TEST(net_socket, close_bad_file_descriptor_v6)
{
  close_bad_file_descriptor(TypeParam::v6());
}


template <typename Protocol>
void broadcast (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);

  bool original, value;
  socket.get_option(sal::net::broadcast(&original));
  socket.set_option(sal::net::broadcast(!original));
  socket.get_option(sal::net::broadcast(&value));
  EXPECT_NE(original, value);
}


// broadcast is valid only for datagram sockets
template <>
void broadcast (const sal::net::ip::tcp_t &)
{}


TYPED_TEST(net_socket, broadcast_v4)
{
  broadcast(TypeParam::v4());
}


TYPED_TEST(net_socket, broadcast_v6)
{
  broadcast(TypeParam::v6());
}


TYPED_TEST(net_socket, broadcast_invalid)
{
  socket_t<TypeParam> socket;
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


template <typename Protocol>
void debug (const Protocol &protocol)
{
#if __sal_os_linux

  // requires CAP_NET_ADMIN or root
  (void)protocol;

#else

  socket_t<Protocol> socket(protocol);

  bool original, value;
  socket.get_option(sal::net::debug(&original));
  socket.set_option(sal::net::debug(!original));
  socket.get_option(sal::net::debug(&value));
  EXPECT_NE(original, value);

#endif
}


TYPED_TEST(net_socket, debug_v4)
{
  debug(TypeParam::v4());
}


TYPED_TEST(net_socket, debug_v6)
{
  debug(TypeParam::v6());
}


TYPED_TEST(net_socket, debug_invalid)
{
  socket_t<TypeParam> socket;
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


template <typename Protocol>
void do_not_route (const Protocol &protocol)
{
#if __sal_os_windows

  // MS providers don't support this option
  (void)protocol;

#else

  socket_t<Protocol> socket(protocol);

  bool original, value;
  socket.get_option(sal::net::do_not_route(&original));
  socket.set_option(sal::net::do_not_route(!original));
  socket.get_option(sal::net::do_not_route(&value));
  EXPECT_NE(original, value);

#endif
}


TYPED_TEST(net_socket, do_not_route_v4)
{
  do_not_route(TypeParam::v4());
}


TYPED_TEST(net_socket, do_not_route_v6)
{
  do_not_route(TypeParam::v6());
}


TYPED_TEST(net_socket, do_not_route_invalid)
{
  socket_t<TypeParam> socket;
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


template <typename Protocol>
void keep_alive (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);

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


// keep_alive is valid only for connection-oriented protocols
template <>
void keep_alive (const sal::net::ip::udp_t &)
{}


TYPED_TEST(net_socket, keep_alive_v4)
{
  keep_alive(TypeParam::v4());
}


TYPED_TEST(net_socket, keep_alive_v6)
{
  keep_alive(TypeParam::v6());
}


TYPED_TEST(net_socket, keep_alive_invalid)
{
  socket_t<TypeParam> socket;
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


template <typename Protocol>
void reuse_address (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);

  bool original, value;
  socket.get_option(sal::net::reuse_address(&original));
  socket.set_option(sal::net::reuse_address(!original));
  socket.get_option(sal::net::reuse_address(&value));
  EXPECT_NE(original, value);
}


TYPED_TEST(net_socket, reuse_address_v4)
{
  reuse_address(TypeParam::v4());
}


TYPED_TEST(net_socket, reuse_address_v6)
{
  reuse_address(TypeParam::v6());
}


TYPED_TEST(net_socket, reuse_address_invalid)
{
  socket_t<TypeParam> socket;
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


template <typename Protocol>
void receive_buffer_size (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);

  int original, value;
  socket.get_option(sal::net::receive_buffer_size(&original));
  socket.set_option(sal::net::receive_buffer_size(2 * original));
  socket.get_option(sal::net::receive_buffer_size(&value));
  EXPECT_NE(original, value);
}


TYPED_TEST(net_socket, receive_buffer_size_v4)
{
  receive_buffer_size(TypeParam::v4());
}


TYPED_TEST(net_socket, receive_buffer_size_v6)
{
  receive_buffer_size(TypeParam::v6());
}


TYPED_TEST(net_socket, receive_buffer_size_invalid)
{
  socket_t<TypeParam> socket;
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


template <typename Protocol>
void receive_low_watermark (const Protocol &protocol)
{
#if __sal_os_windows

  (void)protocol;

#else

  socket_t<Protocol> socket(protocol);

  int original, value;
  socket.get_option(sal::net::receive_low_watermark(&original));
  socket.set_option(sal::net::receive_low_watermark(2 * original));
  socket.get_option(sal::net::receive_low_watermark(&value));
  EXPECT_NE(original, value);

#endif
}


TYPED_TEST(net_socket, receive_low_watermark_v4)
{
  receive_low_watermark(TypeParam::v4());
}


TYPED_TEST(net_socket, receive_low_watermark_v6)
{
  receive_low_watermark(TypeParam::v6());
}


TYPED_TEST(net_socket, receive_low_watermark_invalid)
{
  socket_t<TypeParam> socket;
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


template <typename Protocol>
void send_buffer_size (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);

  int original, value;
  socket.get_option(sal::net::send_buffer_size(&original));
  socket.set_option(sal::net::send_buffer_size(2 * original));
  socket.get_option(sal::net::send_buffer_size(&value));
  EXPECT_NE(original, value);
}


TYPED_TEST(net_socket, send_buffer_size_v4)
{
  send_buffer_size(TypeParam::v4());
}


TYPED_TEST(net_socket, send_buffer_size_v6)
{
  send_buffer_size(TypeParam::v6());
}


TYPED_TEST(net_socket, send_buffer_size_invalid)
{
  socket_t<TypeParam> socket;
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


template <typename Protocol>
void send_low_watermark (const Protocol &protocol)
{
#if __sal_os_windows || __sal_os_linux

  // not changeable on those platforms
  (void)protocol;

#else

  socket_t<Protocol> socket(protocol);

  int original, value;
  socket.get_option(sal::net::send_low_watermark(&original));
  socket.set_option(sal::net::send_low_watermark(2 * original));
  socket.get_option(sal::net::send_low_watermark(&value));
  EXPECT_NE(original, value);

#endif
}


TYPED_TEST(net_socket, send_low_watermark_v4)
{
  send_low_watermark(TypeParam::v4());
}


TYPED_TEST(net_socket, send_low_watermark_v6)
{
  send_low_watermark(TypeParam::v6());
}


TYPED_TEST(net_socket, send_low_watermark_invalid)
{
  socket_t<TypeParam> socket;
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


template <typename Protocol>
void linger (const Protocol &protocol)
{
  using namespace std::chrono_literals;

  socket_t<Protocol> socket(protocol);
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


// linger is valid only for reliable, connection-oriented protocols
template <>
void linger (const sal::net::ip::udp_t &)
{}


TYPED_TEST(net_socket, linger_v4)
{
  linger(TypeParam::v4());
}


TYPED_TEST(net_socket, linger_v6)
{
  linger(TypeParam::v6());
}


TYPED_TEST(net_socket, linger_invalid)
{
  using namespace std::chrono_literals;

  socket_t<TypeParam> socket;
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


template <typename Protocol>
void non_blocking (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);

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


TYPED_TEST(net_socket, non_blocking_v4)
{
  non_blocking(TypeParam::v4());
}


TYPED_TEST(net_socket, non_blocking_v6)
{
  non_blocking(TypeParam::v6());
}


TYPED_TEST(net_socket, non_blocking_invalid)
{
  socket_t<TypeParam> socket;

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


template <typename Protocol>
void available (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);
  EXPECT_EQ(0U, socket.available());
}


TYPED_TEST(net_socket, available_v4)
{
  available(TypeParam::v4());
}


TYPED_TEST(net_socket, available_v6)
{
  available(TypeParam::v6());
}


TYPED_TEST(net_socket, available_invalid)
{
  socket_t<TypeParam> socket;

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


template <typename Protocol>
void bind (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);
  typename Protocol::endpoint_t endpoint(protocol, 0);
  socket.bind(endpoint);

  endpoint = socket.local_endpoint();
  EXPECT_TRUE(endpoint.address().is_unspecified());
  EXPECT_NE(0U, endpoint.port());
}


TYPED_TEST(net_socket, bind_v4)
{
  bind(TypeParam::v4());
}


TYPED_TEST(net_socket, bind_v6)
{
  bind(TypeParam::v6());
}


TYPED_TEST(net_socket, bind_invalid)
{
  socket_t<TypeParam> socket;
  typename TypeParam::endpoint_t endpoint;

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
  socket_t<TypeParam> socket;

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


template <typename Protocol>
void local_endpoint_not_bound (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);
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


TYPED_TEST(net_socket, local_endpoint_not_bound_v4)
{
  local_endpoint_not_bound(TypeParam::v4());
}


TYPED_TEST(net_socket, local_endpoint_not_bound_v6)
{
  local_endpoint_not_bound(TypeParam::v6());
}


TYPED_TEST(net_socket, remote_endpoint_invalid)
{
  socket_t<TypeParam> socket;

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


template <typename Protocol>
void remote_endpoint_not_connected (const Protocol &protocol)
{
  socket_t<Protocol> socket(protocol);
  std::error_code error;
  auto endpoint = socket.remote_endpoint(error);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_TRUE(endpoint.address().is_unspecified());
  EXPECT_EQ(0U, endpoint.port());
}


TYPED_TEST(net_socket, remote_endpoint_not_connected_v4)
{
  remote_endpoint_not_connected(TypeParam::v4());
}


TYPED_TEST(net_socket, remote_endpoint_not_connected_v6)
{
  remote_endpoint_not_connected(TypeParam::v6());
}


template <typename Protocol>
void connect_no_listener (const Protocol &protocol,
  const sal::net::ip::address_t &address)
{
  socket_t<Protocol> socket(protocol);
  typename Protocol::endpoint_t endpoint(address, 7);

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


template <>
void connect_no_listener (const sal::net::ip::udp_t &protocol,
  const sal::net::ip::address_t &address)
{
  socket_t<sal::net::ip::udp_t> socket(protocol);
  sal::net::ip::udp_t::endpoint_t endpoint(address, 7);

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


TYPED_TEST(net_socket, connect_no_listener_v4)
{
  connect_no_listener(TypeParam::v4(),
    sal::net::ip::address_v4_t::loopback()
  );
}


TYPED_TEST(net_socket, connect_no_listener_v6)
{
  connect_no_listener(TypeParam::v6(),
    sal::net::ip::address_v6_t::loopback()
  );
}


template <typename Protocol>
void connect_with_no_pre_open (const Protocol &,
  const sal::net::ip::address_t &address)
{
  socket_t<Protocol> socket;
  typename Protocol::endpoint_t endpoint(address, 7);

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


template <>
void connect_with_no_pre_open (const sal::net::ip::udp_t &,
  const sal::net::ip::address_t &address)
{
  socket_t<sal::net::ip::udp_t> socket;
  sal::net::ip::udp_t::endpoint_t endpoint(address, 7);

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


TYPED_TEST(net_socket, connect_with_no_pre_open_v4)
{
  connect_with_no_pre_open(TypeParam::v4(),
    sal::net::ip::address_v4_t::loopback()
  );
}


TYPED_TEST(net_socket, connect_with_no_pre_open_v6)
{
  connect_with_no_pre_open(TypeParam::v6(),
    sal::net::ip::address_v6_t::loopback()
  );
}


TYPED_TEST(net_socket, shutdown_invalid)
{
  socket_t<TypeParam> socket;
  auto what = socket.shutdown_receive | socket.shutdown_send;

  {
    std::error_code error;
    socket.shutdown(what, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.shutdown(what), std::system_error);
  }
}


template <typename Protocol>
void wait (const Protocol &protocol)
{
  using namespace std::chrono_literals;
  socket_t<Protocol> socket(protocol);
  std::error_code error;

  {
    EXPECT_FALSE(socket.wait(socket.wait_write, 0ms, error));
    EXPECT_FALSE(bool(error));
  }

  {
    EXPECT_FALSE(socket.wait(socket.wait_read, 0ms, error));
    EXPECT_FALSE(bool(error));
  }
}


template <>
void wait (const sal::net::ip::udp_t &protocol)
{
  using namespace std::chrono_literals;
  socket_t<sal::net::ip::udp_t> socket(protocol);
  std::error_code error;

  {
    EXPECT_TRUE(socket.wait(socket.wait_write, 0ms, error));
  }

  {
    EXPECT_FALSE(socket.wait(socket.wait_read, 0ms, error));
  }
}


TYPED_TEST(net_socket, wait_v4)
{
  wait(TypeParam::v4());
}


TYPED_TEST(net_socket, wait_v6)
{
  wait(TypeParam::v6());
}


TYPED_TEST(net_socket, wait_invalid)
{
  using namespace std::chrono_literals;
  socket_t<TypeParam> socket;
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
