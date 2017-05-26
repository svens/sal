#include <sal/net/io_service.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_io_buf
  : public sal_test::fixture
{
  sal::net::io_service_t service;
  sal::net::io_service_t::context_t context = service.make_context();

  auto make_buf ()
  {
    return context.make_buf();
  }
};


TEST_F(net_io_buf, ctor)
{
  auto buf = make_buf();
  EXPECT_EQ(&context, &buf->this_context());

  EXPECT_EQ(buf->data(), buf->begin());
  EXPECT_EQ(buf->head(), buf->begin());
  EXPECT_EQ(buf->tail(), buf->end());

  EXPECT_NE(0U, buf->size());
  EXPECT_EQ(buf->size(), buf->max_size());

  EXPECT_EQ(0U, buf->head_gap());
  EXPECT_EQ(0U, buf->tail_gap());
}


TEST_F(net_io_buf, user_data)
{
  auto buf = make_buf();
  buf->user_data(1);
  EXPECT_EQ(1U, buf->user_data());
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
  auto buf = make_buf();
  EXPECT_THROW(buf->begin(buf->size() + 1), std::logic_error);
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
  auto buf = make_buf();
  EXPECT_THROW(buf->resize(buf->size() + 1), std::logic_error);
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


TEST_F(net_io_buf, reset)
{
  auto buf = make_buf();
  EXPECT_EQ(&context, &buf->this_context());

  buf->begin(1);
  buf->resize(buf->max_size() - 2);

  buf->reset();
  EXPECT_EQ(&context, &buf->this_context());

  EXPECT_EQ(buf->head(), buf->begin());
  EXPECT_EQ(buf->tail(), buf->end());

  EXPECT_NE(0U, buf->size());
  EXPECT_EQ(buf->size(), buf->max_size());

  EXPECT_EQ(0U, buf->head_gap());
  EXPECT_EQ(0U, buf->tail_gap());
}


} // namespace
