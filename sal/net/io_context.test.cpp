#include <sal/net/io_context.hpp>
#include <sal/common.test.hpp>


namespace {


using net_io_context = sal_test::fixture;


TEST_F(net_io_context, make_buf)
{
  sal::net::io_context_t ctx;

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
  sal::net::io_context_t ctx;

  using namespace std::chrono_literals;
  auto buf = ctx.wait(3s);
  /*
  ASSERT_NE(nullptr, buf);

  EXPECT_EQ(buf->head(), buf->begin());
  EXPECT_EQ(buf->tail(), buf->end());
  EXPECT_NE(0U, buf->size());
  EXPECT_EQ(buf->size(), buf->max_size());
  EXPECT_EQ(0U, buf->head_gap());
  EXPECT_EQ(0U, buf->tail_gap());
  */
}


} // namespace
