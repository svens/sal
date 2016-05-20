#include <sal/concurrent_queue.hpp>
#include <sal/common.test.hpp>
#include <chrono>
#include <mutex>
#include <thread>


namespace {


using concurrent_queue = sal_test::fixture;


struct foo
{
  sal::concurrent_queue_hook hook;
  using queue = sal::concurrent_queue<foo, &foo::hook>;
};


TEST_F(concurrent_queue, ctor)
{
  foo::queue q;
  ASSERT_EQ(nullptr, q.try_pop());
}


TEST_F(concurrent_queue, move_ctor_empty)
{
  foo::queue q;
  ASSERT_EQ(nullptr, q.try_pop());

  auto q1 = std::move(q);
  ASSERT_EQ(nullptr, q1.try_pop());
}


TEST_F(concurrent_queue, move_ctor_empty_1)
{
  foo::queue q;

  foo f;
  q.push(&f);
  ASSERT_EQ(&f, q.try_pop());

  auto q1 = std::move(q);
  ASSERT_EQ(nullptr, q1.try_pop());
}


TEST_F(concurrent_queue, move_ctor_single)
{
  foo::queue q;

  foo f;
  q.push(&f);

  auto q1 = std::move(q);
  ASSERT_EQ(&f, q1.try_pop());
  ASSERT_EQ(nullptr, q1.try_pop());
}


TEST_F(concurrent_queue, move_ctor_single_1)
{
  foo::queue q;

  foo f1, f2;
  q.push(&f1);
  q.push(&f2);
  ASSERT_EQ(&f1, q.try_pop());

  auto q1 = std::move(q);
  ASSERT_EQ(&f2, q1.try_pop());
  ASSERT_EQ(nullptr, q1.try_pop());
}


TEST_F(concurrent_queue, move_ctor_multiple)
{
  foo::queue q;

  foo f1, f2;
  q.push(&f1);
  q.push(&f2);

  auto q1 = std::move(q);
  ASSERT_EQ(&f1, q1.try_pop());
  ASSERT_EQ(&f2, q1.try_pop());
  ASSERT_EQ(nullptr, q1.try_pop());
}


TEST_F(concurrent_queue, move_ctor_multiple_1)
{
  foo::queue q;

  foo f1, f2, f3;
  q.push(&f1);
  q.push(&f2);
  q.push(&f3);
  ASSERT_EQ(&f1, q.try_pop());

  auto q1 = std::move(q);
  ASSERT_EQ(&f2, q1.try_pop());
  ASSERT_EQ(&f3, q1.try_pop());
  ASSERT_EQ(nullptr, q1.try_pop());
}


TEST_F(concurrent_queue, move_assign_empty)
{
  foo::queue q1, q2;
  q2 = std::move(q1);
  ASSERT_EQ(nullptr, q2.try_pop());
}


TEST_F(concurrent_queue, move_assign_empty_1)
{
  foo::queue q1, q2;

  foo f1;
  q1.push(&f1);
  ASSERT_EQ(&f1, q1.try_pop());

  q2 = std::move(q1);
  ASSERT_EQ(nullptr, q2.try_pop());
}


TEST_F(concurrent_queue, move_assign_single)
{
  foo::queue q1, q2;

  foo f1;
  q1.push(&f1);

  q2 = std::move(q1);
  ASSERT_EQ(&f1, q2.try_pop());
  ASSERT_EQ(nullptr, q2.try_pop());
}


TEST_F(concurrent_queue, move_assign_single_1)
{
  foo::queue q1, q2;

  foo f1, f2;
  q1.push(&f1);
  q1.push(&f2);
  ASSERT_EQ(&f1, q1.try_pop());

  q2 = std::move(q1);
  ASSERT_EQ(&f2, q2.try_pop());
  ASSERT_EQ(nullptr, q2.try_pop());
}


TEST_F(concurrent_queue, move_assign_multiple)
{
  foo::queue q1, q2;

  foo f1, f2;
  q1.push(&f1);
  q1.push(&f2);

  q2 = std::move(q1);
  ASSERT_EQ(&f1, q2.try_pop());
  ASSERT_EQ(&f2, q2.try_pop());
  ASSERT_EQ(nullptr, q2.try_pop());
}


TEST_F(concurrent_queue, move_assign_multiple_1)
{
  foo::queue q1, q2;

  foo f1, f2, f3;
  q1.push(&f1);
  q1.push(&f2);
  q1.push(&f3);
  ASSERT_EQ(&f1, q1.try_pop());

  q2 = std::move(q1);
  ASSERT_EQ(&f2, q2.try_pop());
  ASSERT_EQ(&f3, q2.try_pop());
  ASSERT_EQ(nullptr, q2.try_pop());
}


TEST_F(concurrent_queue, single_push_pop)
{
  foo::queue q;

  foo f;
  q.push(&f);
  ASSERT_EQ(&f, q.try_pop());

  ASSERT_EQ(nullptr, q.try_pop());
}


TEST_F(concurrent_queue, multiple_push_pop)
{
  foo::queue q;

  foo f1, f2, f3;
  q.push(&f1);
  q.push(&f2);
  q.push(&f3);

  ASSERT_EQ(&f1, q.try_pop());
  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_EQ(&f3, q.try_pop());
  ASSERT_EQ(nullptr, q.try_pop());
}


TEST_F(concurrent_queue, interleaved_push_pop)
{
  foo::queue q;

  // push 1, 2
  foo f1, f2;
  q.push(&f1);
  q.push(&f2);

  // pop 1
  ASSERT_EQ(&f1, q.try_pop());

  // push 3
  foo f3;
  q.push(&f3);

  // pop 2, push 2
  ASSERT_EQ(&f2, q.try_pop());
  q.push(&f2);

  // pop 3
  ASSERT_EQ(&f3, q.try_pop());

  // pop 2
  ASSERT_EQ(&f2, q.try_pop());

  // pop nil
  ASSERT_EQ(nullptr, q.try_pop());
}


} // namespace
