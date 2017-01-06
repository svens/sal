#include <sal/queue.hpp>
#include <sal/common.test.hpp>
#include <thread>


namespace {


template <typename SyncPolicy>
struct test
  : public sal_test::with_type<SyncPolicy>
{
  using queue_t = sal::queue_t<int, SyncPolicy>;
  queue_t queue{};
};


using types = testing::Types<
  sal::no_sync_t,
  sal::spsc_sync_t
>;


TYPED_TEST_CASE_P(test);


TYPED_TEST_P(test, ctor)
{
  int i;
  ASSERT_EQ(false, this->queue.try_pop(&i));
}


TYPED_TEST_P(test, move_ctor_empty)
{
  int i;
  ASSERT_EQ(false, this->queue.try_pop(&i));

  auto q = std::move(this->queue);
  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, move_ctor_empty_1)
{
  this->queue.push(1);

  int i = 0;
  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(1, i);

  auto q = std::move(this->queue);
  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, move_ctor_single)
{
  this->queue.push(1);

  auto q = std::move(this->queue);

  int i = 0;
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(1, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, move_ctor_single_1)
{
  this->queue.push(1);
  this->queue.push(2);

  int i = 0;
  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(1, i);

  auto q = std::move(this->queue);
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, move_ctor_multiple)
{
  this->queue.push(1);
  this->queue.push(2);

  auto q = std::move(this->queue);

  int i = 0;
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(1, i);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, move_ctor_multiple_1)
{
  this->queue.push(1);
  this->queue.push(2);
  this->queue.push(3);

  int i;
  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(1, i);

  auto q = std::move(this->queue);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(3, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, move_assign_empty)
{
  typename TestFixture::queue_t q;
  q = std::move(this->queue);

  int i;
  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, move_assign_empty_1)
{
  typename TestFixture::queue_t q;

  this->queue.push(1);

  int i = 0;
  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(1, i);

  q = std::move(this->queue);
  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, move_assign_single)
{
  typename TestFixture::queue_t q;

  this->queue.push(1);

  q = std::move(this->queue);

  int i = 0;
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(1, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, move_assign_single_1)
{
  typename TestFixture::queue_t q;

  this->queue.push(1);
  this->queue.push(2);

  int i = 0;
  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(1, i);

  q = std::move(this->queue);
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, move_assign_multiple)
{
  typename TestFixture::queue_t q;

  this->queue.push(1);
  this->queue.push(2);

  q = std::move(this->queue);

  int i = 0;
  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(1, i);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, move_assign_multiple_1)
{
  typename TestFixture::queue_t q;

  this->queue.push(1);
  this->queue.push(2);
  this->queue.push(3);

  int i = 0;
  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(1, i);

  q = std::move(this->queue);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(true, q.try_pop(&i));
  EXPECT_EQ(3, i);

  ASSERT_EQ(false, q.try_pop(&i));
}


TYPED_TEST_P(test, single_push_pop)
{
  this->queue.push(1);

  int i = 0;
  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(1, i);

  ASSERT_EQ(false, this->queue.try_pop(&i));
}


TYPED_TEST_P(test, multiple_push_pop)
{
  this->queue.push(1);
  this->queue.push(2);
  this->queue.push(3);

  int i = 0;
  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(1, i);
  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(2, i);
  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(3, i);

  ASSERT_EQ(false, this->queue.try_pop(&i));
}


TYPED_TEST_P(test, interleaved_push_pop)
{
  this->queue.push(1);
  this->queue.push(2);

  int i = 0;
  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(1, i);

  this->queue.push(3);

  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(2, i);

  this->queue.push(2);

  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(3, i);

  ASSERT_EQ(true, this->queue.try_pop(&i));
  EXPECT_EQ(2, i);

  ASSERT_EQ(false, this->queue.try_pop(&i));
}


REGISTER_TYPED_TEST_CASE_P(test,
  ctor,

  move_ctor_empty,
  move_ctor_empty_1,
  move_ctor_single,
  move_ctor_single_1,
  move_ctor_multiple,
  move_ctor_multiple_1,

  move_assign_empty,
  move_assign_empty_1,
  move_assign_single,
  move_assign_single_1,
  move_assign_multiple,
  move_assign_multiple_1,

  single_push_pop,
  multiple_push_pop,
  interleaved_push_pop
);


INSTANTIATE_TYPED_TEST_CASE_P(queue, test, types);


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
