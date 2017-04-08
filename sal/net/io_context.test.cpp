#include <sal/net/io_context.hpp>
#include <sal/net/io_service.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/common.test.hpp>


namespace {


using namespace std::chrono_literals;


struct net_io_context
  : public sal_test::fixture
{
  sal::net::io_service_t service;
  sal::net::io_context_t context = service.make_context();

  auto make_buf ()
  {
    return context.make_buf();
  }

  auto make_buf (const std::string &content)
  {
    auto io_buf = make_buf();
    io_buf->resize(content.size());
    std::memcpy(io_buf->data(), content.data(), content.size());
    return io_buf;
  }

  using protocol_t = sal::net::ip::udp_t;
  using socket_t = protocol_t::socket_t;

  auto endpoint () const noexcept
  {
    return socket_t::endpoint_t(
      sal::net::ip::address_v4_t::loopback(),
      8188
    );
  }

  auto make_socket ()
  {
    socket_t socket(endpoint());
    service.associate(socket);
    return socket;
  }
};


TEST_F(net_io_context, make_buf)
{
  auto buf = make_buf();
  EXPECT_EQ(buf->head(), buf->begin());
  EXPECT_EQ(buf->tail(), buf->end());
  EXPECT_NE(0U, buf->size());
  EXPECT_EQ(buf->size(), buf->max_size());
  EXPECT_EQ(0U, buf->head_gap());
  EXPECT_EQ(0U, buf->tail_gap());
}


TEST_F(net_io_context, try_get_empty)
{
  EXPECT_EQ(nullptr, context.try_get());
}


TEST_F(net_io_context, try_get_not_empty)
{
  auto socket = make_socket();

  // because try_get does not wait actually, we need 2 packets:
  // first get() does polling and populates try_get list
  socket.async_send(make_buf("first"));
  socket.async_send(make_buf("second"));

  EXPECT_NE(nullptr, context.get());
  EXPECT_NE(nullptr, context.try_get());
}


TEST_F(net_io_context, get_invalid)
{
  auto p_svc = std::make_unique<sal::net::io_service_t>();
  auto ctx = p_svc->make_context();
  p_svc.reset();

  std::error_code error;
  ctx.get(error);
  EXPECT_FALSE(!error);

  EXPECT_THROW(ctx.get(), std::system_error);
}


#if !__sal_os_windows

TEST_F(net_io_context, get_invalid_time)
{
  std::error_code error;
  context.get((std::chrono::hours::max)(), error);
  EXPECT_FALSE(!error);

  EXPECT_THROW(
    context.get((std::chrono::hours::max)()),
    std::system_error
  );
}

#endif // !__sal_os_windows


TEST_F(net_io_context, get_empty)
{
  EXPECT_EQ(nullptr, context.get(1ms));
}


TEST_F(net_io_context, get_not_empty)
{
  auto socket = make_socket();
  socket.async_send(make_buf(case_name));
  EXPECT_NE(nullptr, context.get());
}


TEST_F(net_io_context, reclaim_empty)
{
  EXPECT_EQ(0U, context.reclaim());
  EXPECT_EQ(nullptr, context.try_get());
}


TEST_F(net_io_context, reclaim_not_empty)
{
  auto socket = make_socket();
  socket.async_send(make_buf("first"));
  socket.async_send(make_buf("second"));
  socket.async_send(make_buf("third"));

  EXPECT_NE(nullptr, context.get());
  EXPECT_EQ(2U, context.reclaim());

  EXPECT_EQ(nullptr, context.try_get());
}


} // namespace
