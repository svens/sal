#include <sal/intrusive_queue.hpp>
#include <sal/common.test.hpp>
#include <array>
#include <thread>


namespace {


using namespace std::chrono_literals;


template <typename QueueHook>
struct intrusive_queue
  : public sal_test::with_type<QueueHook>
{
  struct foo_t
  {
    typename QueueHook::intrusive_queue_hook_t hook;
  };
  using queue_t = sal::intrusive_queue_t<foo_t, QueueHook, &foo_t::hook>;
  queue_t queue{};
};


using sync_types = testing::Types<
  sal::no_sync_t,
  sal::spsc_sync_t,
  sal::mpsc_sync_t
>;


TYPED_TEST_CASE(intrusive_queue, sync_types);


TYPED_TEST(intrusive_queue, ctor)
{
  ASSERT_EQ(nullptr, this->queue.try_pop());
  EXPECT_TRUE(this->queue.empty());
}


TYPED_TEST(intrusive_queue, move_ctor_empty)
{
  ASSERT_EQ(nullptr, this->queue.try_pop());

  auto q = std::move(this->queue);
  ASSERT_EQ(nullptr, q.try_pop());
  EXPECT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, move_ctor_empty_1)
{
  typename TestFixture::foo_t f;
  this->queue.push(&f);
  EXPECT_FALSE(this->queue.empty());

  ASSERT_EQ(&f, this->queue.try_pop());
  EXPECT_TRUE(this->queue.empty());

  auto q = std::move(this->queue);
  EXPECT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  EXPECT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, move_ctor_single)
{
  typename TestFixture::foo_t f;
  this->queue.push(&f);

  auto q = std::move(this->queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, move_ctor_single_1)
{
  typename TestFixture::foo_t f1, f2;
  this->queue.push(&f1);
  this->queue.push(&f2);
  ASSERT_FALSE(this->queue.empty());

  ASSERT_EQ(&f1, this->queue.try_pop());
  ASSERT_FALSE(this->queue.empty());

  auto q = std::move(this->queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, move_ctor_multiple)
{
  typename TestFixture::foo_t f1, f2;
  this->queue.push(&f1);
  this->queue.push(&f2);

  auto q = std::move(this->queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f1, q.try_pop());
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, move_ctor_multiple_1)
{
  typename TestFixture::foo_t f1, f2, f3;
  this->queue.push(&f1);
  this->queue.push(&f2);
  this->queue.push(&f3);
  ASSERT_EQ(&f1, this->queue.try_pop());

  auto q = std::move(this->queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f3, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, move_assign_empty)
{
  typename TestFixture::queue_t q;
  q = std::move(this->queue);
  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, move_assign_empty_1)
{
  typename TestFixture::queue_t q;

  typename TestFixture::foo_t f1;
  this->queue.push(&f1);
  ASSERT_EQ(&f1, this->queue.try_pop());

  q = std::move(this->queue);
  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, move_assign_single)
{
  typename TestFixture::queue_t q;

  typename TestFixture::foo_t f1;
  this->queue.push(&f1);

  q = std::move(this->queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f1, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, move_assign_single_1)
{
  typename TestFixture::queue_t q;

  typename TestFixture::foo_t f1, f2;
  this->queue.push(&f1);
  this->queue.push(&f2);
  ASSERT_EQ(&f1, this->queue.try_pop());

  q = std::move(this->queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, move_assign_multiple)
{
  typename TestFixture::queue_t q;

  typename TestFixture::foo_t f1, f2;
  this->queue.push(&f1);
  this->queue.push(&f2);

  q = std::move(this->queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f1, q.try_pop());
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, move_assign_multiple_1)
{
  typename TestFixture::queue_t q;

  typename TestFixture::foo_t f1, f2, f3;
  this->queue.push(&f1);
  this->queue.push(&f2);
  this->queue.push(&f3);
  ASSERT_EQ(&f1, this->queue.try_pop());

  q = std::move(this->queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f3, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TYPED_TEST(intrusive_queue, single_push_pop)
{
  typename TestFixture::foo_t f;
  ASSERT_TRUE(this->queue.empty());

  this->queue.push(&f);
  ASSERT_FALSE(this->queue.empty());

  ASSERT_EQ(&f, this->queue.try_pop());
  ASSERT_TRUE(this->queue.empty());

  ASSERT_EQ(nullptr, this->queue.try_pop());
  ASSERT_TRUE(this->queue.empty());
}


TYPED_TEST(intrusive_queue, multiple_push_pop)
{
  typename TestFixture::foo_t f1, f2, f3;
  ASSERT_TRUE(this->queue.empty());
  this->queue.push(&f1);
  this->queue.push(&f2);
  this->queue.push(&f3);
  ASSERT_FALSE(this->queue.empty());

  ASSERT_EQ(&f1, this->queue.try_pop());
  ASSERT_FALSE(this->queue.empty());

  ASSERT_EQ(&f2, this->queue.try_pop());
  ASSERT_FALSE(this->queue.empty());

  ASSERT_EQ(&f3, this->queue.try_pop());
  ASSERT_TRUE(this->queue.empty());

  ASSERT_EQ(nullptr, this->queue.try_pop());
  ASSERT_TRUE(this->queue.empty());
}


TYPED_TEST(intrusive_queue, interleaved_push_pop)
{
  // push 1, 2
  typename TestFixture::foo_t f1, f2;
  this->queue.push(&f1);
  this->queue.push(&f2);
  ASSERT_FALSE(this->queue.empty());

  // pop 1
  ASSERT_EQ(&f1, this->queue.try_pop());
  ASSERT_FALSE(this->queue.empty());

  // push 3
  typename TestFixture::foo_t f3;
  this->queue.push(&f3);
  ASSERT_FALSE(this->queue.empty());

  // pop 2, push 2
  ASSERT_EQ(&f2, this->queue.try_pop());
  this->queue.push(&f2);
  ASSERT_FALSE(this->queue.empty());

  // pop 3
  ASSERT_EQ(&f3, this->queue.try_pop());
  ASSERT_FALSE(this->queue.empty());

  // pop 2
  ASSERT_EQ(&f2, this->queue.try_pop());
  ASSERT_TRUE(this->queue.empty());

  // pop nil
  ASSERT_EQ(nullptr, this->queue.try_pop());
  ASSERT_TRUE(this->queue.empty());
}


TEST(intrusive_queue, spsc)
{
  using fixture_t = intrusive_queue<sal::spsc_sync_t>;
  std::array<fixture_t::foo_t, 10'000> data{};
  fixture_t::queue_t queue;

  // consumer
  auto consumer = std::thread([&]
  {
    for (auto i = 0U;  i != data.size();  /**/)
    {
      if (auto p = queue.try_pop())
      {
        ASSERT_EQ(&data[i], p);
        ++i;
      }
      std::this_thread::yield();
    }
  });

  std::this_thread::sleep_for(1ms);

  // producer
  for (auto &p: data)
  {
    queue.push(&p);
    std::this_thread::yield();
  }

  consumer.join();
}


TEST(intrusive_queue, mpsc)
{
  using fixture_t = intrusive_queue<sal::mpsc_sync_t>;
  std::array<fixture_t::foo_t, 10'000> data{};
  fixture_t::queue_t queue;

  // consumer
  auto consumer = std::thread([&]
  {
    for (auto i = 0U;  i != data.size();  /**/)
    {
      if (auto p = queue.try_pop())
      {
        ASSERT_EQ(&data[i], p);
        ++i;
      }
      std::this_thread::yield();
    }
  });

  std::this_thread::sleep_for(1ms);

  // producer
  for (auto &p: data)
  {
    queue.push(&p);
    std::this_thread::yield();
  }

  consumer.join();
}


} // namespace
