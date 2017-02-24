#include <sal/net/io_context.hpp>
#include <sal/net/io_service.hpp>
#include <sal/common.test.hpp>


#if __sal_os_windows


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
}


TEST_F(net_io_context, try_get_not_empty)
{
}


TEST_F(net_io_context, get_empty)
{
}


TEST_F(net_io_context, get_not_empty)
{
}


TEST_F(net_io_context, wait_empty)
{
}


TEST_F(net_io_context, wait_not_empty)
{
}


} // namespace


#endif // __sal_os_windows
