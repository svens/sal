#include <sal/net/async/service.hpp>
#include <sal/net/internet.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_async_service
  : public sal_test::fixture
{
  sal::net::async::service_t service{};
};


TEST_F(net_async_service, io_pool_size)
{
  EXPECT_EQ(0U, service.io_pool_size());

  (void)service.make_io();
  EXPECT_LT(0U, service.io_pool_size());
}


TEST_F(net_async_service, io_pool_size_increases_after_exhaustion_and_alloc)
{
  EXPECT_EQ(0U, service.io_pool_size());

  // alloc first
  std::vector<sal::net::async::io_ptr> io_list;
  io_list.emplace_back(service.make_io());
  auto size_after_first_alloc = service.io_pool_size();
  EXPECT_LT(0U, size_after_first_alloc);

  // exhaust pool
  while (io_list.size() < size_after_first_alloc)
  {
    io_list.emplace_back(service.make_io());
  }
  EXPECT_EQ(size_after_first_alloc, service.io_pool_size());

  // alloc again to increase pool
  io_list.emplace_back(service.make_io());
  EXPECT_LT(size_after_first_alloc, service.io_pool_size());
}


TEST_F(net_async_service, io_pool_size_remains_same_after_exhaustion_and_release)
{
  EXPECT_EQ(0U, service.io_pool_size());

  // alloc first
  std::vector<sal::net::async::io_ptr> io_list;
  io_list.emplace_back(service.make_io());
  auto size_after_first_alloc = service.io_pool_size();
  EXPECT_LT(0U, size_after_first_alloc);

  // exhaust pool
  while (io_list.size() < size_after_first_alloc)
  {
    io_list.emplace_back(service.make_io());
  }
  EXPECT_EQ(size_after_first_alloc, service.io_pool_size());

  // release all and alloc again
  io_list.clear();
  io_list.emplace_back(service.make_io());
  EXPECT_EQ(size_after_first_alloc, service.io_pool_size());
}


} // namespace
