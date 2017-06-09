#include <sal/net/async_service.hpp>
#include <sal/net/internet.hpp>
#include <sal/common.test.hpp>


namespace {


using namespace std::chrono_literals;


// async_service_t {{{1
template <typename Socket>
struct net_async_service
  : public sal_test::with_type<Socket>
{
  sal::net::async_service_t svc;
};

using types = ::testing::Types<
  sal::net::ip::udp_t::socket_t,
  sal::net::ip::tcp_t::socket_t,
  sal::net::ip::tcp_t::acceptor_t
>;

TYPED_TEST_CASE(net_async_service, types);


TYPED_TEST(net_async_service, associate)
{
  TypeParam socket(TypeParam::protocol_t::v4());
  socket.associate(this->svc);
}


TYPED_TEST(net_async_service, associate_already_associated)
{
  TypeParam socket(TypeParam::protocol_t::v4());
  ASSERT_NO_THROW(socket.associate(this->svc));

  {
    std::error_code error;
    socket.associate(this->svc, error);
    EXPECT_EQ(sal::net::socket_errc::already_associated, error);
  }

  {
    EXPECT_THROW(socket.associate(this->svc), std::system_error);
  }
}


TYPED_TEST(net_async_service, associate_invalid_socket)
{
  TypeParam socket;

  {
    std::error_code error;
    socket.associate(this->svc, error);
    EXPECT_EQ(std::errc::invalid_argument, error);
  }

  {
    EXPECT_THROW(socket.associate(this->svc), std::system_error);
  }
}


TYPED_TEST(net_async_service, make_context)
{
  auto ctx = this->svc.make_context();
  (void)ctx;
}


TYPED_TEST(net_async_service, make_context_too_small_completion_count)
{
  auto ctx = this->svc.make_context(0);
  (void)ctx;
}


TYPED_TEST(net_async_service, make_context_too_big_completion_count)
{
  auto ctx = this->svc.make_context((std::numeric_limits<size_t>::max)());
  (void)ctx;
}


// async_service_t::io_t {{{1
struct net_async_io
  : public sal_test::fixture
{
  sal::net::async_service_t svc;
  sal::net::async_service_t::context_t ctx = svc.make_context();
};


TEST_F(net_async_io, ctor)
{
  auto io = ctx.make_io();
  EXPECT_EQ(&ctx, &io->this_context());

  EXPECT_EQ(io->data(), io->begin());
  EXPECT_EQ(io->head(), io->begin());
  EXPECT_EQ(io->tail(), io->end());

  EXPECT_NE(0U, io->size());
  EXPECT_EQ(io->size(), io->max_size())
    << "size=" << io->size()
    << "; max_size=" << io->max_size();

  EXPECT_EQ(0U, io->head_gap());
  EXPECT_EQ(0U, io->tail_gap());
}


TEST_F(net_async_io, user_data)
{
  auto io = ctx.make_io();
  io->user_data(1);
  EXPECT_EQ(1U, io->user_data());
}


TEST_F(net_async_io, head_gap)
{
  auto io = ctx.make_io();
  io->begin(1);

  EXPECT_EQ(1U, io->head_gap());
  EXPECT_EQ(0U, io->tail_gap());

  EXPECT_NE(io->head(), io->begin());
  EXPECT_EQ(io->tail(), io->end());

  EXPECT_NE(0U, io->size());
  EXPECT_NE(0U, io->max_size());
  EXPECT_EQ(io->size() + 1, io->max_size());
}


TEST_F(net_async_io, head_gap_invalid)
{
  auto io = ctx.make_io();
  EXPECT_THROW(io->begin(io->size() + 1), std::logic_error);
}


TEST_F(net_async_io, tail_gap)
{
  auto io = ctx.make_io();
  io->resize(io->max_size() - 1);

  EXPECT_EQ(0U, io->head_gap());
  EXPECT_EQ(1U, io->tail_gap());

  EXPECT_EQ(io->head(), io->begin());
  EXPECT_NE(io->tail(), io->end());

  EXPECT_NE(0U, io->size());
  EXPECT_NE(0U, io->max_size());
  EXPECT_EQ(io->size() + 1, io->max_size());
}


TEST_F(net_async_io, tail_gap_invalid)
{
  auto io = ctx.make_io();
  EXPECT_THROW(io->resize(io->size() + 1), std::logic_error);
}


TEST_F(net_async_io, head_and_tail_gap)
{
  auto io = ctx.make_io();
  io->begin(1);
  io->resize(io->max_size() - 2);

  EXPECT_EQ(1U, io->head_gap());
  EXPECT_EQ(1U, io->tail_gap());

  EXPECT_NE(io->head(), io->begin());
  EXPECT_NE(io->tail(), io->end());

  EXPECT_NE(0U, io->size());
  EXPECT_NE(0U, io->max_size());
  EXPECT_EQ(io->size(), io->max_size() - 2);
}


TEST_F(net_async_io, reset)
{
  auto io = ctx.make_io();
  EXPECT_EQ(&ctx, &io->this_context());

  io->begin(1);
  io->resize(io->max_size() - 2);

  io->reset();
  EXPECT_EQ(&ctx, &io->this_context());

  EXPECT_EQ(io->head(), io->begin());
  EXPECT_EQ(io->tail(), io->end());

  EXPECT_NE(0U, io->size());
  EXPECT_EQ(io->size(), io->max_size());

  EXPECT_EQ(0U, io->head_gap());
  EXPECT_EQ(0U, io->tail_gap());
}


// async_service_t::context_t {{{1
struct net_async_context
  : public sal_test::fixture
{
  sal::net::async_service_t svc;

  using protocol_t = sal::net::ip::udp_t;
  using socket_t = protocol_t::socket_t;

  auto make_socket ()
  {
    socket_t socket(
      socket_t::endpoint_t(
        sal::net::ip::address_v4_t::loopback(),
        8188
      )
    );
    socket.associate(svc);
    return socket;
  }

  auto make_buf (const std::string &content,
    sal::net::async_service_t::context_t &ctx)
  {
    auto io = ctx.make_io();
    io->resize(content.size());
    std::memcpy(io->data(), content.data(), content.size());
    return io;
  }
};


TEST_F(net_async_context, make_io)
{
  auto ctx = svc.make_context();

  auto first = ctx.make_io();
  std::set<sal::net::io_ptr> io;
  for (auto i = 0U;  i < 2048;  ++i)
  {
    io.emplace(ctx.make_io());
  }

  EXPECT_EQ(2048U, io.size());
  EXPECT_EQ(0U, io.count(first));
}


TEST_F(net_async_context, make_io_reuse)
{
  auto ctx = svc.make_context();

  std::set<sal::net::async_service_t::io_t *> io;
  for (auto i = 0U;  i < 2048;  ++i)
  {
    io.emplace(ctx.make_io().get());
  }
  EXPECT_GT(2048U, io.size());

  EXPECT_EQ(1024U, io.size())
    << "*** internal knowledge ***\n"
    << "(remove if any impl deviates)";
}


TEST_F(net_async_context, try_get_empty)
{
  auto ctx = svc.make_context();
  EXPECT_EQ(nullptr, ctx.try_get());
}


TEST_F(net_async_context, try_get_not_empty)
{
  auto ctx = svc.make_context();
  auto socket = make_socket();

  // try_get does not actually wait
  // need 2 packets: 1st poll() does populate internal list and 2nd try_get()
  // will consume from that list
  socket.async_send(make_buf("first", ctx));
  socket.async_send(make_buf("second", ctx));

  EXPECT_NE(nullptr, ctx.poll());
  EXPECT_NE(nullptr, ctx.try_get());
}


TEST_F(net_async_context, try_poll_empty)
{
  auto ctx = svc.make_context();
  EXPECT_EQ(nullptr, ctx.try_poll());
}


TEST_F(net_async_context, try_poll_not_empty)
{
  auto ctx = svc.make_context();
  auto socket = make_socket();
  socket.async_send(make_buf(case_name, ctx));
  EXPECT_NE(nullptr, ctx.try_poll());
}


TEST_F(net_async_context, try_poll_with_destroyed_service)
{
  auto p_svc = std::make_unique<sal::net::async_service_t>();
  auto ctx = p_svc->make_context();
  p_svc.reset();

  std::error_code error;
  ctx.try_poll(error);
  EXPECT_TRUE(!error);

  EXPECT_NO_THROW(ctx.try_poll());
}


TEST_F(net_async_context, poll_empty)
{
  auto ctx = svc.make_context();
  EXPECT_EQ(nullptr, ctx.poll(1ms));
}


TEST_F(net_async_context, poll_not_empty)
{
  auto ctx = svc.make_context();
  auto socket = make_socket();
  socket.async_send(make_buf(case_name, ctx));
  EXPECT_NE(nullptr, ctx.poll(1s));
}


TEST_F(net_async_context, poll_with_destroyed_service)
{
  auto p_svc = std::make_unique<sal::net::async_service_t>();
  auto ctx = p_svc->make_context();
  p_svc.reset();

  std::error_code error;
  ctx.poll(1ms, error);
  EXPECT_TRUE(!error);

  EXPECT_NO_THROW(ctx.poll(1ms));
}


TEST_F(net_async_context, reclaim_empty)
{
  auto ctx = svc.make_context();
  EXPECT_EQ(0U, ctx.reclaim());
  EXPECT_EQ(nullptr, ctx.try_get());
}


TEST_F(net_async_context, reclaim_not_empty)
{
  auto ctx = svc.make_context();
  auto socket = make_socket();
  socket.async_send(make_buf("first", ctx));
  socket.async_send(make_buf("second", ctx));
  socket.async_send(make_buf("third", ctx));

  EXPECT_NE(nullptr, ctx.try_poll());
  EXPECT_EQ(2U, ctx.reclaim());

  EXPECT_EQ(nullptr, ctx.try_get());
}


// }}}1


} // namespace
