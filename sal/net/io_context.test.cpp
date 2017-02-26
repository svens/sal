#include <sal/net/io_context.hpp>
#include <sal/net/io_service.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_io_context
  : public sal_test::fixture
{
  static auto &context ()
  {
    static sal::net::io_service_t svc;
    static sal::net::io_context_t ctx = svc.make_context();
    return ctx;
  }

  auto make_buf ()
  {
    return context().make_buf();
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
  EXPECT_EQ(nullptr, context().try_get());
}


TEST_F(net_io_context, try_get_not_empty)
{
  // TODO
}


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
  context().reclaim();
  EXPECT_EQ(nullptr, context().try_get());
}


TEST_F(net_io_context, reclaim_not_empty)
{
  // TODO
  context().reclaim();
  EXPECT_EQ(nullptr, context().try_get());
}


} // namespace
