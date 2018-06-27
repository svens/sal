#include <sal/net/async/io.hpp>
#include <sal/net/async/service.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_async_io
  : public sal_test::fixture
{
  sal::net::async::service_t service{10};
};


TEST_F(net_async_io, ctor)
{
  auto io = service.make_io();
  EXPECT_EQ(io.data(), io.begin());
  EXPECT_EQ(io.head(), io.begin());
  EXPECT_EQ(io.tail(), io.end());

  EXPECT_EQ(0U, io.head_gap());
  EXPECT_EQ(0U, io.tail_gap());

  EXPECT_NE(0U, io.size());
  EXPECT_EQ(io.max_size(), io.size());
}


TEST_F(net_async_io, user_data)
{
  auto io = service.make_io(&service);
  EXPECT_EQ(&service, io.user_data());

  io.user_data(&io);
  EXPECT_EQ(&io, io.user_data());
}


TEST_F(net_async_io, head_gap)
{
  auto io = service.make_io();
  io.head_gap(1);

  EXPECT_EQ(1U, io.head_gap());
  EXPECT_EQ(0U, io.tail_gap());

  EXPECT_NE(io.head(), io.begin());
  EXPECT_EQ(io.tail(), io.end());

  EXPECT_NE(0U, io.size());
  EXPECT_NE(0U, io.max_size());
  EXPECT_EQ(io.max_size(), io.size() + 1);
}


TEST_F(net_async_io, head_gap_invalid)
{
  auto io = service.make_io();
  EXPECT_THROW(io.head_gap(io.max_size() + 1), std::logic_error);
}


TEST_F(net_async_io, tail_gap)
{
  auto io = service.make_io();
  io.tail_gap(1);

  EXPECT_EQ(0U, io.head_gap());
  EXPECT_EQ(1U, io.tail_gap());

  EXPECT_EQ(io.head(), io.begin());
  EXPECT_NE(io.tail(), io.end());

  EXPECT_NE(0U, io.size());
  EXPECT_NE(0U, io.max_size());
  EXPECT_EQ(io.max_size(), io.size() + 1);
}


TEST_F(net_async_io, tail_gap_invalid)
{
  auto io = service.make_io();
  EXPECT_THROW(io.tail_gap(io.max_size() + 1), std::logic_error);
}


TEST_F(net_async_io, head_and_tail_gap)
{
  auto io = service.make_io();
  io.head_gap(1);
  io.tail_gap(1);

  EXPECT_EQ(1U, io.head_gap());
  EXPECT_EQ(1U, io.tail_gap());

  EXPECT_NE(io.head(), io.begin());
  EXPECT_NE(io.tail(), io.end());

  EXPECT_NE(0U, io.size());
  EXPECT_NE(0U, io.max_size());
  EXPECT_EQ(io.max_size(), io.size() + 2);
}


TEST_F(net_async_io, resize)
{
  auto io = service.make_io();

  io.resize(io.max_size() - 1);
  EXPECT_EQ(0U, io.head_gap());
  EXPECT_EQ(1U, io.tail_gap());
}


TEST_F(net_async_io, resize_invalid)
{
  auto io = service.make_io();
  EXPECT_THROW(io.resize(io.max_size() + 1), std::logic_error);
}


TEST_F(net_async_io, reset)
{
  auto io = service.make_io();

  io.user_data(&io);
  io.head_gap(1);
  io.tail_gap(1);
  io.reset();

  EXPECT_EQ(nullptr, io.user_data());

  EXPECT_EQ(io.head(), io.begin());
  EXPECT_EQ(io.tail(), io.end());

  EXPECT_NE(0U, io.size());
  EXPECT_EQ(io.max_size(), io.size());

  EXPECT_EQ(0U, io.head_gap());
  EXPECT_EQ(0U, io.tail_gap());
}


TEST_F(net_async_io, release)
{
  auto io = service.make_io();
  EXPECT_TRUE(static_cast<bool>(io));

  io.release();
  EXPECT_FALSE(static_cast<bool>(io));
}


} // namespace
