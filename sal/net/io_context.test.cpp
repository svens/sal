#include <sal/net/io_context.hpp>
#include <sal/net/io_service.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_io_context
  : public sal_test::fixture
{
  sal::net::io_service_t service;
  sal::net::io_context_t context = service.make_context();

  auto make_buf ()
  {
    return context.make_buf();
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
  // sal::net::ip::udp_t::socket_t socket(sal::net::ip::udp_t::v4());
  // TODO
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
  // TODO
}


TEST_F(net_io_context, get_not_empty)
{
  // TODO
}


TEST_F(net_io_context, reclaim_empty)
{
  context.reclaim();
  EXPECT_EQ(nullptr, context.try_get());
}


TEST_F(net_io_context, reclaim_not_empty)
{
  // TODO
  context.reclaim();
  EXPECT_EQ(nullptr, context.try_get());
}


} // namespace
