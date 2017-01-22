#include <sal/net/io_buf.hpp>
#include <sal/net/io_context.hpp>
#include <sal/net/io_service.hpp>
#include <sal/common.test.hpp>


#include <sal/net/ip/udp.hpp>


namespace {


struct net_io_buf
  : public sal_test::fixture
{
  static auto &service ()
  {
    static sal::net::io_service_t svc;
    return svc;
  }


  static auto &context ()
  {
    static sal::net::io_context_t ctx = service().make_context();
    return ctx;
  }

  auto make_buf ()
  {
    return context().make_buf();
  }
};


TEST_F(net_io_buf, ctor)
{
  auto buf = make_buf();
  EXPECT_EQ(&context(), &buf->this_context());

  EXPECT_EQ(buf->head(), buf->begin());
  EXPECT_EQ(buf->tail(), buf->end());

  EXPECT_NE(0U, buf->size());
  EXPECT_EQ(buf->size(), buf->max_size());

  EXPECT_EQ(0U, buf->head_gap());
  EXPECT_EQ(0U, buf->tail_gap());
}


TEST_F(net_io_buf, socket_data)
{
  auto buf = make_buf();
  EXPECT_EQ(0U, buf->socket_data());
}


TEST_F(net_io_buf, head_gap)
{
  auto buf = make_buf();
  buf->begin(1);

  EXPECT_EQ(1U, buf->head_gap());
  EXPECT_EQ(0U, buf->tail_gap());

  EXPECT_NE(buf->head(), buf->begin());
  EXPECT_EQ(buf->tail(), buf->end());

  EXPECT_NE(0U, buf->size());
  EXPECT_NE(0U, buf->max_size());
  EXPECT_EQ(buf->size() + 1, buf->max_size());
}


TEST_F(net_io_buf, head_gap_invalid)
{
#if !defined(NDEBUG)
  auto buf = make_buf();
  EXPECT_THROW(buf->begin(buf->size() + 1), std::logic_error);
#endif
}


TEST_F(net_io_buf, tail_gap)
{
  auto buf = make_buf();
  buf->resize(buf->max_size() - 1);

  EXPECT_EQ(0U, buf->head_gap());
  EXPECT_EQ(1U, buf->tail_gap());

  EXPECT_EQ(buf->head(), buf->begin());
  EXPECT_NE(buf->tail(), buf->end());

  EXPECT_NE(0U, buf->size());
  EXPECT_NE(0U, buf->max_size());
  EXPECT_EQ(buf->size() + 1, buf->max_size());
}


TEST_F(net_io_buf, tail_gap_invalid)
{
#if !defined(NDEBUG)
  auto buf = make_buf();
  EXPECT_THROW(buf->resize(buf->size() + 1), std::logic_error);
#endif
}


TEST_F(net_io_buf, head_and_tail_gap)
{
  auto buf = make_buf();
  buf->begin(1);
  buf->resize(buf->max_size() - 2);

  EXPECT_EQ(1U, buf->head_gap());
  EXPECT_EQ(1U, buf->tail_gap());

  EXPECT_NE(buf->head(), buf->begin());
  EXPECT_NE(buf->tail(), buf->end());

  EXPECT_NE(0U, buf->size());
  EXPECT_NE(0U, buf->max_size());
  EXPECT_EQ(buf->size(), buf->max_size() - 2);
}


TEST_F(net_io_buf, clear)
{
  auto buf = make_buf();
  EXPECT_EQ(&context(), &buf->this_context());

  buf->begin(1);
  buf->resize(buf->max_size() - 2);

  buf->clear();
  EXPECT_EQ(&context(), &buf->this_context());

  EXPECT_EQ(buf->head(), buf->begin());
  EXPECT_EQ(buf->tail(), buf->end());

  EXPECT_NE(0U, buf->size());
  EXPECT_EQ(buf->size(), buf->max_size());

  EXPECT_EQ(0U, buf->head_gap());
  EXPECT_EQ(0U, buf->tail_gap());
}


} // namespace
