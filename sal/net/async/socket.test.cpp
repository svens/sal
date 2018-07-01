#include <sal/net/async/basic_socket.hpp>
#include <sal/net/internet.hpp>
#include <sal/common.test.hpp>


namespace {


template <typename Socket>
struct net_async_socket
  : public sal_test::with_type<Socket>
{
  sal::net::async::service_t service{10};
};

using socket_types = ::testing::Types<
  sal::net::ip::udp_t::async_socket_t
>;

TYPED_TEST_CASE(net_async_socket, socket_types);


TYPED_TEST(net_async_socket, associate)
{
  TypeParam socket(TypeParam::protocol_t::v4());
  EXPECT_NO_THROW(socket.associate(TestFixture::service, 1, 1));
}


TYPED_TEST(net_async_socket, associate_already_associated)
{
  TypeParam socket(TypeParam::protocol_t::v4());
  ASSERT_NO_THROW(socket.associate(TestFixture::service, 1, 1));

  {
    std::error_code error;
    socket.associate(TestFixture::service, 1, 1, error);
    EXPECT_EQ(sal::net::socket_errc::already_associated, error);
  }

  {
    EXPECT_THROW(
      socket.associate(TestFixture::service, 1, 1),
      std::system_error
    );
  }
}


TYPED_TEST(net_async_socket, associate_invalid_socket)
{
  TypeParam socket;

  {
    std::error_code error;
    socket.associate(TestFixture::service, 1, 1, error);
    EXPECT_EQ(std::errc::bad_file_descriptor, error);
  }

  {
    EXPECT_THROW(
      socket.associate(TestFixture::service, 1, 1),
      std::system_error
    );
  }
}


TYPED_TEST(net_async_socket, context)
{
  TypeParam socket(TypeParam::protocol_t::v4());
  socket.associate(TestFixture::service, 1, 1);

  socket.context(&socket);
  EXPECT_EQ(&socket, socket.template context<decltype(socket)>());
  EXPECT_EQ(nullptr, socket.template context<decltype(TestFixture::service)>());
}


TYPED_TEST(net_async_socket, context_none)
{
  TypeParam socket(TypeParam::protocol_t::v4());
  socket.associate(TestFixture::service, 1, 1);

  EXPECT_EQ(nullptr, socket.template context<decltype(socket)>());
  EXPECT_EQ(nullptr, socket.template context<decltype(TestFixture::service)>());
}


TYPED_TEST(net_async_socket, context_before_associate)
{
  if constexpr (!sal::is_debug_build)
  {
    return;
  }

  TypeParam socket;
  EXPECT_THROW(
    socket.context(&socket),
    std::logic_error
  );
}


} // namespace
