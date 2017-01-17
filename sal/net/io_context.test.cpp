#include <sal/net/io_context.hpp>
#include <sal/net/io_service.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_io_context
  : public sal_test::fixture
{
  sal::net::io_service_t svc;
  sal::net::io_context_t ctx = svc.make_context();
};


TEST_F(net_io_context, make_buf)
{
  auto buf = ctx.make_buf();
  EXPECT_EQ(buf->head(), buf->begin());
  EXPECT_EQ(buf->tail(), buf->end());
  EXPECT_NE(0U, buf->size());
  EXPECT_EQ(buf->size(), buf->max_size());
  EXPECT_EQ(0U, buf->head_gap());
  EXPECT_EQ(0U, buf->tail_gap());
}


TEST_F(net_io_context, wait)
{
  using namespace std::chrono_literals;
  auto buf = ctx.wait(3s);
  EXPECT_EQ(nullptr, buf);
}


} // namespace
