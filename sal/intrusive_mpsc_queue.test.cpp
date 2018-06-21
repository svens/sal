#include <sal/intrusive_mpsc_queue.hpp>
#include <sal/common.test.hpp>


namespace {


struct intrusive_mpsc_queue
  : public sal_test::fixture
{
  struct foo_t
  {
    sal::intrusive_mpsc_queue_hook_t<foo_t> hook;
  };
  using queue_t = sal::intrusive_mpsc_queue_t<foo_t, &foo_t::hook>;
  queue_t queue{};
};


TEST_F(intrusive_mpsc_queue, ctor)
{
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(nullptr, queue.try_pop());
}


TEST_F(intrusive_mpsc_queue, move_ctor_empty)
{
  ASSERT_EQ(nullptr, queue.try_pop());

  auto q = std::move(queue);
  ASSERT_EQ(nullptr, q.try_pop());
  EXPECT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, move_ctor_empty_1)
{
  foo_t f;
  queue.push(&f);
  EXPECT_FALSE(queue.empty());
  ASSERT_EQ(&f, queue.try_pop());
  EXPECT_TRUE(queue.empty());

  auto q = std::move(queue);
  ASSERT_EQ(nullptr, q.try_pop());
  EXPECT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, move_ctor_single)
{
  foo_t f;
  queue.push(&f);

  auto q = std::move(queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, move_ctor_single_1)
{
  foo_t f1, f2;
  queue.push(&f1);
  queue.push(&f2);
  ASSERT_FALSE(queue.empty());

  ASSERT_EQ(&f1, queue.try_pop());
  ASSERT_FALSE(queue.empty());

  auto q = std::move(queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, move_ctor_multiple)
{
  foo_t f1, f2;
  queue.push(&f1);
  queue.push(&f2);

  auto q = std::move(queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f1, q.try_pop());
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, move_ctor_multiple_1)
{
  foo_t f1, f2, f3;
  queue.push(&f1);
  queue.push(&f2);
  queue.push(&f3);
  ASSERT_EQ(&f1, queue.try_pop());

  auto q = std::move(queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f3, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, move_assign_empty)
{
  queue_t q;
  q = std::move(queue);
  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, move_assign_empty_1)
{
  queue_t q;

  foo_t f1;
  queue.push(&f1);
  ASSERT_EQ(&f1, queue.try_pop());

  q = std::move(queue);
  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, move_assign_single)
{
  queue_t q;

  foo_t f1;
  queue.push(&f1);

  q = std::move(queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f1, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, move_assign_single_1)
{
  queue_t q;

  foo_t f1, f2;
  queue.push(&f1);
  queue.push(&f2);
  ASSERT_EQ(&f1, queue.try_pop());

  q = std::move(queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, move_assign_multiple)
{
  queue_t q;

  foo_t f1, f2;
  queue.push(&f1);
  queue.push(&f2);

  q = std::move(queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f1, q.try_pop());
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, move_assign_multiple_1)
{
  queue_t q;

  foo_t f1, f2, f3;
  queue.push(&f1);
  queue.push(&f2);
  queue.push(&f3);
  ASSERT_EQ(&f1, queue.try_pop());

  q = std::move(queue);
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_FALSE(q.empty());

  ASSERT_EQ(&f3, q.try_pop());
  ASSERT_TRUE(q.empty());

  ASSERT_EQ(nullptr, q.try_pop());
  ASSERT_TRUE(q.empty());
}


TEST_F(intrusive_mpsc_queue, single_push_pop)
{
  foo_t f;
  ASSERT_TRUE(queue.empty());

  queue.push(&f);
  ASSERT_FALSE(queue.empty());

  ASSERT_EQ(&f, queue.try_pop());
  ASSERT_TRUE(queue.empty());

  ASSERT_EQ(nullptr, queue.try_pop());
  ASSERT_TRUE(queue.empty());
}


TEST_F(intrusive_mpsc_queue, multiple_push_pop)
{
  foo_t f1, f2, f3;
  ASSERT_TRUE(queue.empty());
  queue.push(&f1);
  queue.push(&f2);
  queue.push(&f3);
  ASSERT_FALSE(queue.empty());

  ASSERT_EQ(&f1, queue.try_pop());
  ASSERT_FALSE(queue.empty());

  ASSERT_EQ(&f2, queue.try_pop());
  ASSERT_FALSE(queue.empty());

  ASSERT_EQ(&f3, queue.try_pop());
  ASSERT_TRUE(queue.empty());

  ASSERT_EQ(nullptr, queue.try_pop());
  ASSERT_TRUE(queue.empty());
}


TEST_F(intrusive_mpsc_queue, interleaved_push_pop)
{
  // push 1, 2
  foo_t f1, f2;
  queue.push(&f1);
  queue.push(&f2);
  ASSERT_FALSE(queue.empty());

  // pop 1
  ASSERT_EQ(&f1, queue.try_pop());
  ASSERT_FALSE(queue.empty());

  // push 3
  foo_t f3;
  queue.push(&f3);
  ASSERT_FALSE(queue.empty());

  // pop 2, push 2
  ASSERT_EQ(&f2, queue.try_pop());
  queue.push(&f2);
  ASSERT_FALSE(queue.empty());

  // pop 3
  ASSERT_EQ(&f3, queue.try_pop());
  ASSERT_FALSE(queue.empty());

  // pop 2
  ASSERT_EQ(&f2, queue.try_pop());
  ASSERT_TRUE(queue.empty());

  // pop nil
  ASSERT_EQ(nullptr, queue.try_pop());
  ASSERT_TRUE(queue.empty());
}


} // namespace
