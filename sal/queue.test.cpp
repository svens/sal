#include <sal/queue.hpp>
#include <sal/common.test.hpp>
#include <thread>


namespace {


template <typename SyncPolicy>
struct queue
  : public sal_test::with_type<SyncPolicy>
{
  using queue_t = sal::queue_t<int, SyncPolicy>;
  queue_t queue_{};
};


using sync_types = testing::Types<
  sal::no_sync_t,
  sal::spsc_sync_t
>;


TYPED_TEST_CASE(queue, sync_types);


TYPED_TEST(queue, ctor)
{
  int i;
  ASSERT_EQ(false, this->queue_.try_pop(&i));
}


TYPED_TEST(queue, move_ctor_empty)
{
  int i;
  ASSERT_EQ(false, this->queue_.try_pop(&i));

  auto q = std::move(this->queue_);
  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, move_ctor_empty_1)
{
  this->queue_.push(1);

  int i = 0;
  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(1, i);

  auto q = std::move(this->queue_);
  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, move_ctor_single)
{
  this->queue_.push(1);

  auto q = std::move(this->queue_);

  int i = 0;
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(1, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, move_ctor_single_1)
{
  this->queue_.push(1);
  this->queue_.push(2);

  int i = 0;
  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(1, i);

  auto q = std::move(this->queue_);
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, move_ctor_multiple)
{
  this->queue_.push(1);
  this->queue_.push(2);

  auto q = std::move(this->queue_);

  int i = 0;
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(1, i);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, move_ctor_multiple_1)
{
  this->queue_.push(1);
  this->queue_.push(2);
  this->queue_.push(3);

  int i;
  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(1, i);

  auto q = std::move(this->queue_);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(3, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, move_assign_empty)
{
  typename TestFixture::queue_t q;
  q = std::move(this->queue_);

  int i;
  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, move_assign_empty_1)
{
  typename TestFixture::queue_t q;

  this->queue_.push(1);

  int i = 0;
  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(1, i);

  q = std::move(this->queue_);
  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, move_assign_single)
{
  typename TestFixture::queue_t q;

  this->queue_.push(1);

  q = std::move(this->queue_);

  int i = 0;
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(1, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, move_assign_single_1)
{
  typename TestFixture::queue_t q;

  this->queue_.push(1);
  this->queue_.push(2);

  int i = 0;
  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(1, i);

  q = std::move(this->queue_);
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, move_assign_multiple)
{
  typename TestFixture::queue_t q;

  this->queue_.push(1);
  this->queue_.push(2);

  q = std::move(this->queue_);

  int i = 0;
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(1, i);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, move_assign_multiple_1)
{
  typename TestFixture::queue_t q;

  this->queue_.push(1);
  this->queue_.push(2);
  this->queue_.push(3);

  int i = 0;
  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(1, i);

  q = std::move(this->queue_);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(3, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST(queue, single_push_pop)
{
  this->queue_.push(1);

  int i = 0;
  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(1, i);

  ASSERT_EQ(false, this->queue_.try_pop(&i));
}


TYPED_TEST(queue, multiple_push_pop)
{
  this->queue_.push(1);
  this->queue_.push(2);
  this->queue_.push(3);

  int i = 0;
  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(1, i);
  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(2, i);
  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(3, i);

  ASSERT_EQ(false, this->queue_.try_pop(&i));
}


TYPED_TEST(queue, interleaved_push_pop)
{
  this->queue_.push(1);
  this->queue_.push(2);

  int i = 0;
  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(1, i);

  this->queue_.push(3);

  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(2, i);

  this->queue_.push(2);

  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(3, i);

  ASSERT_EQ(true, this->queue_.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(false, this->queue_.try_pop(&i));
}


TEST(queue, single_consumer_single_producer)
{
  // FYI, this test is almost meaningless, just for fun
  sal::queue_t<int, sal::spsc_sync_t> queue{};

  // consumer
  auto consumer = std::thread([&queue]
  {
    int i = 0, prev = 0;
    while (i != -1)
    {
      if (queue.try_pop(&i) && i != -1)
      {
        EXPECT_EQ(prev + 1, i);
        prev = i;
      }
      std::this_thread::yield();
    }
  });

  // producer
  for (int i = 1;  i < 10000;  ++i)
  {
    queue.push(i);
  }
  queue.push(-1);

  consumer.join();
}


} // namespace
