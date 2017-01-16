#include <sal/net/io_service.hpp>
#include <sal/common.test.hpp>


namespace {


using net_io_buf = sal_test::fixture;


TEST_F(net_io_buf, ctor)
{
  sal::net::io_buf_t buf;

  EXPECT_EQ(buf.head(), buf.begin());
  EXPECT_EQ(buf.tail(), buf.end());

  EXPECT_NE(0U, buf.size());
  EXPECT_EQ(buf.size(), buf.max_size());

  EXPECT_EQ(0U, buf.head_gap());
  EXPECT_EQ(0U, buf.tail_gap());
}


TEST_F(net_io_buf, user_data)
{
  sal::net::io_buf_t buf;
  EXPECT_EQ(0U, buf.user_data());

  buf.user_data(1);
  EXPECT_EQ(1U, buf.user_data());

  buf.clear();
  EXPECT_EQ(0U, buf.user_data());
}


TEST_F(net_io_buf, head_gap)
{
  sal::net::io_buf_t buf;
  buf.begin(1);

  EXPECT_EQ(1U, buf.head_gap());
  EXPECT_EQ(0U, buf.tail_gap());

  EXPECT_NE(buf.head(), buf.begin());
  EXPECT_EQ(buf.tail(), buf.end());

  EXPECT_NE(0U, buf.size());
  EXPECT_NE(0U, buf.max_size());
  EXPECT_EQ(buf.size() + 1, buf.max_size());
}


TEST_F(net_io_buf, head_gap_invalid)
{
#if !defined(NDEBUG)
  sal::net::io_buf_t buf;
  EXPECT_THROW(buf.begin(buf.size() + 1), std::logic_error);
#endif
}


TEST_F(net_io_buf, tail_gap)
{
  sal::net::io_buf_t buf;
  buf.resize(buf.max_size() - 1);

  EXPECT_EQ(0U, buf.head_gap());
  EXPECT_EQ(1U, buf.tail_gap());

  EXPECT_EQ(buf.head(), buf.begin());
  EXPECT_NE(buf.tail(), buf.end());

  EXPECT_NE(0U, buf.size());
  EXPECT_NE(0U, buf.max_size());
  EXPECT_EQ(buf.size() + 1, buf.max_size());
}


TEST_F(net_io_buf, tail_gap_invalid)
{
#if !defined(NDEBUG)
  sal::net::io_buf_t buf;
  EXPECT_THROW(buf.resize(buf.size() + 1), std::logic_error);
#endif
}


TEST_F(net_io_buf, head_and_tail_gap)
{
  sal::net::io_buf_t buf;
  buf.begin(1);
  buf.resize(buf.max_size() - 2);

  EXPECT_EQ(1U, buf.head_gap());
  EXPECT_EQ(1U, buf.tail_gap());

  EXPECT_NE(buf.head(), buf.begin());
  EXPECT_NE(buf.tail(), buf.end());

  EXPECT_NE(0U, buf.size());
  EXPECT_NE(0U, buf.max_size());
  EXPECT_EQ(buf.size(), buf.max_size() - 2);
}


TEST_F(net_io_buf, clear)
{
  sal::net::io_buf_t buf;
  buf.begin(1);
  buf.resize(buf.max_size() - 2);

  buf.clear();

  EXPECT_EQ(buf.head(), buf.begin());
  EXPECT_EQ(buf.tail(), buf.end());

  EXPECT_NE(0U, buf.size());
  EXPECT_EQ(buf.size(), buf.max_size());

  EXPECT_EQ(0U, buf.head_gap());
  EXPECT_EQ(0U, buf.tail_gap());
}


#if 0
using net_io_service = sal_test::fixture;

TEST_F(net_io_service, run)
{
  sal::net::io_context_t ctx;
  ctx.test();
}
#endif


} // namespace
