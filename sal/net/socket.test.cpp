#include <sal/net/socket.hpp>
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


TYPED_TEST(net_socket, ctor_move_v4)
{
  socket_t<TypeParam> a(TypeParam::v4());
  EXPECT_TRUE(a.is_open());
  auto b{std::move(a)};
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TYPED_TEST(net_socket, ctor_move_v6)
{
  socket_t<TypeParam> a(TypeParam::v6());
  EXPECT_TRUE(a.is_open());
  auto b{std::move(a)};
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
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


TYPED_TEST(net_socket, ctor_protocol_and_handle_v4)
{
  auto handle = sal::net::socket_base_t::no_handle - 1;
  socket_t<TypeParam> socket(TypeParam::v4(), handle);
  EXPECT_EQ(handle, socket.native_handle());

  std::error_code ignored;
  socket.close(ignored);
}


TYPED_TEST(net_socket, ctor_protocol_and_handle_v6)
{
  auto handle = sal::net::socket_base_t::no_handle - 1;
  socket_t<TypeParam> socket(TypeParam::v6(), handle);
  EXPECT_EQ(handle, socket.native_handle());

  std::error_code ignored;
  socket.close(ignored);
}


TYPED_TEST(net_socket, assign_move_v4)
{
  socket_t<TypeParam> a(TypeParam::v4()), b;
  EXPECT_TRUE(a.is_open());
  EXPECT_FALSE(b.is_open());

  auto handle = a.native_handle();
  b = std::move(a);
  EXPECT_EQ(handle, b.native_handle());
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
}


TYPED_TEST(net_socket, assign_move_v6)
{
  socket_t<TypeParam> a(TypeParam::v6()), b;
  EXPECT_TRUE(a.is_open());
  EXPECT_FALSE(b.is_open());

  auto handle = a.native_handle();
  b = std::move(a);
  EXPECT_EQ(handle, b.native_handle());
  EXPECT_TRUE(b.is_open());
  EXPECT_FALSE(a.is_open());
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


TYPED_TEST(net_socket, open_already_open_v4)
{
  socket_t<TypeParam> socket(TypeParam::v4());

  {
    std::error_code error;
    socket.open(TypeParam::v4(), error);
    EXPECT_EQ(sal::net::socket_errc_t::already_open, error);
    EXPECT_TRUE(socket.is_open());
  }

  {
    EXPECT_THROW(socket.open(TypeParam::v4()), std::system_error);
    EXPECT_TRUE(socket.is_open());
  }
}


TYPED_TEST(net_socket, open_already_open_v6)
{
  socket_t<TypeParam> socket(TypeParam::v6());

  {
    std::error_code error;
    socket.open(TypeParam::v6(), error);
    EXPECT_EQ(sal::net::socket_errc_t::already_open, error);
    EXPECT_TRUE(socket.is_open());
  }

  {
    EXPECT_THROW(socket.open(TypeParam::v6()), std::system_error);
    EXPECT_TRUE(socket.is_open());
  }
}


TYPED_TEST(net_socket, assign_v4)
{
  socket_t<TypeParam> socket;

  auto h = sal::net::socket_base_t::no_handle - 1;
  socket.assign(TypeParam::v4(), h);
  EXPECT_TRUE(socket.is_open());
  EXPECT_EQ(h, socket.native_handle());

  std::error_code ignored;
  socket.close(ignored);
}


TYPED_TEST(net_socket, assign_v6)
{
  socket_t<TypeParam> socket;

  auto h = sal::net::socket_base_t::no_handle - 1;
  socket.assign(TypeParam::v6(), h);
  EXPECT_TRUE(socket.is_open());
  EXPECT_EQ(h, socket.native_handle());

  std::error_code ignored;
  socket.close(ignored);
}


TYPED_TEST(net_socket, assign_not_closed_v4)
{
  socket_t<TypeParam> socket(TypeParam::v4());
  auto h = sal::net::socket_base_t::no_handle - 1;

  {
    std::error_code error;
    socket.assign(TypeParam::v4(), h, error);
    EXPECT_EQ(sal::net::socket_errc_t::already_open, error);
  }

  {
    EXPECT_THROW(socket.assign(TypeParam::v4(), h), std::system_error);
  }
}


TYPED_TEST(net_socket, assign_not_closed_v6)
{
  socket_t<TypeParam> socket(TypeParam::v6());
  auto h = sal::net::socket_base_t::no_handle - 1;

  {
    std::error_code error;
    socket.assign(TypeParam::v6(), h, error);
    EXPECT_EQ(sal::net::socket_errc_t::already_open, error);
  }

  {
    EXPECT_THROW(socket.assign(TypeParam::v6(), h), std::system_error);
  }
}


TYPED_TEST(net_socket, assign_no_handle_v4)
{
  socket_t<TypeParam> socket;
  auto h = sal::net::socket_base_t::no_handle;

  {
    std::error_code error;
    socket.assign(TypeParam::v4(), h, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.assign(TypeParam::v4(), h), std::system_error);
  }
}


TYPED_TEST(net_socket, assign_no_handle_v6)
{
  socket_t<TypeParam> socket;
  auto h = sal::net::socket_base_t::no_handle;

  {
    std::error_code error;
    socket.assign(TypeParam::v6(), h, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(socket.assign(TypeParam::v6(), h), std::system_error);
  }
}


TYPED_TEST(net_socket, close_v4)
{
  socket_t<TypeParam> socket;

  socket.open(TypeParam::v4());
  EXPECT_TRUE(socket.is_open());

  socket.close();
  EXPECT_FALSE(socket.is_open());
}


TYPED_TEST(net_socket, close_v6)
{
  socket_t<TypeParam> socket;

  socket.open(TypeParam::v6());
  EXPECT_TRUE(socket.is_open());

  socket.close();
  EXPECT_FALSE(socket.is_open());
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


TYPED_TEST(net_socket, close_bad_file_descriptor_v4)
{
  socket_t<TypeParam> socket(TypeParam::v4(),
    sal::net::socket_base_t::no_handle - 1
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


TYPED_TEST(net_socket, close_bad_file_descriptor_v6)
{
  socket_t<TypeParam> socket(TypeParam::v6(),
    sal::net::socket_base_t::no_handle - 1
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


TYPED_TEST(net_socket, reuse_address_v4)
{
  socket_t<TypeParam> socket(TypeParam::v4());
  bool value{false};

  socket.get_option(sal::net::reuse_address(&value));
  EXPECT_FALSE(value);

  socket.set_option(sal::net::reuse_address(true));

  socket.get_option(sal::net::reuse_address(&value));
  EXPECT_TRUE(value);
}


TYPED_TEST(net_socket, reuse_address_v6)
{
  socket_t<TypeParam> socket(TypeParam::v6());
  bool value{false};

  socket.get_option(sal::net::reuse_address(&value));
  EXPECT_FALSE(value);

  socket.set_option(sal::net::reuse_address(true));

  socket.get_option(sal::net::reuse_address(&value));
  EXPECT_TRUE(value);
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


TYPED_TEST(net_socket, receive_buffer_size_v4)
{
  socket_t<TypeParam> socket(TypeParam::v4());
  int value{};

  socket.get_option(sal::net::receive_buffer_size(&value));
  EXPECT_NE(0, value);

  socket.set_option(sal::net::receive_buffer_size(64 * 1024));

  socket.get_option(sal::net::receive_buffer_size(&value));
  EXPECT_NE(0, value);
}


TYPED_TEST(net_socket, receive_buffer_size_v6)
{
  socket_t<TypeParam> socket(TypeParam::v6());
  int value{};

  socket.get_option(sal::net::receive_buffer_size(&value));
  EXPECT_NE(0, value);

  socket.set_option(sal::net::receive_buffer_size(64 * 1024));

  socket.get_option(sal::net::receive_buffer_size(&value));
  EXPECT_NE(0, value);
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
void test_linger (const Protocol &protocol)
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


#if __sal_os_windows

// linger is valid only for reliable, connection-oriented protocols
template <>
void test_linger (const sal::net::ip::udp_t &)
{}

#endif


TYPED_TEST(net_socket, linger_v4)
{
  test_linger(TypeParam::v4());
}


TYPED_TEST(net_socket, linger_v6)
{
  test_linger(TypeParam::v6());
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


} // namespace
