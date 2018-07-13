#include <sal/net/async/worker.hpp>
#include <sal/net/async/service.hpp>
#include <sal/common.test.hpp>


namespace {


using namespace std::chrono_literals;


struct net_async_worker
  : public sal_test::fixture
{
  sal::net::async::service_t service{};
};


TEST_F(net_async_worker, try_get)
{
  auto worker = service.make_worker(10);
  auto io = worker.try_get();
  EXPECT_TRUE(!io);
}


TEST_F(net_async_worker, poll)
{
  auto worker = service.make_worker(10);
  auto io = worker.poll(10ms);
  EXPECT_TRUE(!io);
}


TEST_F(net_async_worker, reclaim)
{
  auto worker = service.make_worker(10);
  EXPECT_EQ(0U, worker.reclaim());
}


} // namespace
