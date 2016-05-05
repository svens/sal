#include <sal/concurrent_queue.hpp>
#include <sal/common.test.hpp>
#include <chrono>
#include <mutex>
#include <thread>


namespace {


template <typename T>
using test = sal_test::with_type<T>;


template <typename UsePolicy>
using queue = sal::concurrent_queue<int, UsePolicy>;


TYPED_TEST_CASE_P(test);


template <typename UsePolicy>
constexpr bool is_lock_free () noexcept
{
  return false;
}


template <>
constexpr bool is_lock_free<sal::spsc> () noexcept
{
  return true;
}


template <>
constexpr bool is_lock_free<sal::mpsc> () noexcept
{
  return true;
}


TYPED_TEST_P(test, lock_free)
{
  typename queue<TypeParam>::node stub;
  queue<TypeParam> q(&stub);
  EXPECT_EQ(is_lock_free<TypeParam>(), q.is_lock_free());
}


TYPED_TEST_P(test, ctor)
{
  typename queue<TypeParam>::node stub;
  queue<TypeParam> q(&stub);
  EXPECT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_ctor_empty)
{
  typename queue<TypeParam>::node stub;
  queue<TypeParam> q(&stub);
  EXPECT_EQ(nullptr, q.try_pop());

  auto q1 = std::move(q);
  EXPECT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_empty_1)
{
  typename queue<TypeParam>::node stub;
  queue<TypeParam> q(&stub);

  typename queue<TypeParam>::node n1(1);
  q.push(&n1);

  auto n = q.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);

  auto q1 = std::move(q);
  EXPECT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_single)
{
  typename queue<TypeParam>::node stub;
  queue<TypeParam> q(&stub);

  typename queue<TypeParam>::node n1(1);
  q.push(&n1);

  auto q1 = std::move(q);
  auto n = q1.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);
  EXPECT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_single_1)
{
  typename queue<TypeParam>::node stub;
  queue<TypeParam> q(&stub);

  typename queue<TypeParam>::node n1(1), n2(2);
  q.push(&n1);
  q.push(&n2);

  auto n = q.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);

  auto q1 = std::move(q);
  n = q1.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(2, n->data);
  EXPECT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_multiple)
{
  typename queue<TypeParam>::node stub;
  queue<TypeParam> q(&stub);

  typename queue<TypeParam>::node n1(1), n2(2);
  q.push(&n1);
  q.push(&n2);

  auto q1 = std::move(q);
  auto n = q1.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);
  n = q1.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(2, n->data);
  EXPECT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_multiple_1)
{
  typename queue<TypeParam>::node stub;
  queue<TypeParam> q(&stub);

  typename queue<TypeParam>::node n1(1), n2(2), n3(3);
  q.push(&n1);
  q.push(&n2);
  q.push(&n3);

  auto n = q.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);

  auto q1 = std::move(q);
  n = q1.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(2, n->data);
  n = q1.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(3, n->data);
  EXPECT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_assign_empty)
{
  typename queue<TypeParam>::node stub1;
  queue<TypeParam> q1(&stub1);

  typename queue<TypeParam>::node stub2;
  queue<TypeParam> q2(&stub2);
  q2 = std::move(q1);
  EXPECT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, move_assign_empty_1)
{
  typename queue<TypeParam>::node stub1;
  queue<TypeParam> q1(&stub1);

  typename queue<TypeParam>::node n1(1);
  q1.push(&n1);
  auto n = q1.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);

  typename queue<TypeParam>::node stub2;
  queue<TypeParam> q2(&stub2);
  q2 = std::move(q1);
  EXPECT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, move_assign_single)
{
  typename queue<TypeParam>::node stub1;
  queue<TypeParam> q1(&stub1);

  typename queue<TypeParam>::node n1(1);
  q1.push(&n1);

  typename queue<TypeParam>::node stub2;
  queue<TypeParam> q2(&stub2);
  q2 = std::move(q1);

  auto n = q2.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);

  EXPECT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, move_assign_single_1)
{
  typename queue<TypeParam>::node stub1;
  queue<TypeParam> q1(&stub1);

  typename queue<TypeParam>::node n1(1), n2(2);
  q1.push(&n1);
  q1.push(&n2);

  auto n = q1.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);

  typename queue<TypeParam>::node stub2;
  queue<TypeParam> q2(&stub2);
  q2 = std::move(q1);

  n = q2.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(2, n->data);

  EXPECT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, move_assign_multiple)
{
  typename queue<TypeParam>::node stub1;
  queue<TypeParam> q1(&stub1);

  typename queue<TypeParam>::node n1(1), n2(2);
  q1.push(&n1);
  q1.push(&n2);

  typename queue<TypeParam>::node stub2;
  queue<TypeParam> q2(&stub2);
  q2 = std::move(q1);

  auto n = q2.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);

  n = q2.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(2, n->data);

  EXPECT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, move_assign_multiple_1)
{
  typename queue<TypeParam>::node stub1;
  queue<TypeParam> q1(&stub1);

  typename queue<TypeParam>::node n1(1), n2(2), n3(3);
  q1.push(&n1);
  q1.push(&n2);
  q1.push(&n3);

  auto n = q1.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);

  typename queue<TypeParam>::node stub2;
  queue<TypeParam> q2(&stub2);
  q2 = std::move(q1);

  n = q2.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(2, n->data);

  n = q2.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(3, n->data);

  EXPECT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, single_push_pop)
{
  typename queue<TypeParam>::node stub;
  queue<TypeParam> q(&stub);

  typename queue<TypeParam>::node x(1);
  q.push(&x);

  auto n = q.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);

  EXPECT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, multiple_push_pop)
{
  typename queue<TypeParam>::node stub;
  queue<TypeParam> q(&stub);

  typename queue<TypeParam>::node n1(1), n2(2), n3(3);
  q.push(&n1);
  q.push(&n2);
  q.push(&n3);

  auto n = q.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);

  n = q.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(2, n->data);

  n = q.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(3, n->data);

  n = q.try_pop();
  EXPECT_EQ(nullptr, n);
}


TYPED_TEST_P(test, interleaved_push_pop)
{
  typename queue<TypeParam>::node stub;
  queue<TypeParam> q(&stub);

  // push 1, 2
  typename queue<TypeParam>::node n1(1), n2(2);
  q.push(&n1);
  q.push(&n2);

  // pop 1
  auto n = q.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(1, n->data);

  // push 3
  typename queue<TypeParam>::node n3(3);
  q.push(&n3);

  // pop 2, push 2
  n = q.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(2, n->data);
  q.push(n);

  // pop 3
  n = q.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(3, n->data);

  // pop 2
  n = q.try_pop();
  ASSERT_NE(nullptr, n);
  EXPECT_EQ(2, n->data);

  // pop nil
  n = q.try_pop();
  EXPECT_EQ(nullptr, n);
}


REGISTER_TYPED_TEST_CASE_P(test,
  lock_free,
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


using types = testing::Types<sal::spsc, sal::mpsc, sal::spmc, sal::mpmc>;
INSTANTIATE_TYPED_TEST_CASE_P(concurrent_queue, test, types);


} // namespace
