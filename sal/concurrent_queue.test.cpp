#include <sal/concurrent_queue.hpp>
#include <sal/common.test.hpp>
#include <chrono>
#include <mutex>
#include <thread>


namespace {


template <typename T>
using test = sal_test::with_type<T>;


TYPED_TEST_CASE_P(test);


struct foo
{
  sal::concurrent_queue_hook<foo> hook;

  template <typename UsePolicy>
  using queue = sal::concurrent_queue<foo, &foo::hook, UsePolicy>;
};


template <typename UsePolicy>
constexpr bool is_lock_free () noexcept
{
  return false;
}


template <>
constexpr bool is_lock_free<sal::mpsc> () noexcept
{
  return true;
}


template <>
constexpr bool is_lock_free<sal::spsc> () noexcept
{
  return true;
}


TYPED_TEST_P(test, lock_free)
{
  foo::queue<TypeParam> q;
  EXPECT_EQ(is_lock_free<TypeParam>(), q.is_lock_free());
}


TYPED_TEST_P(test, ctor)
{
  foo::queue<TypeParam> q;
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_ctor_empty)
{
  foo::queue<TypeParam> q;
  ASSERT_EQ(nullptr, q.try_pop());

  auto q1 = std::move(q);
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_empty_1)
{
  foo::queue<TypeParam> q;

  foo f;
  q.push(&f);
  ASSERT_EQ(&f, q.try_pop());
  f.hook = nullptr;
  ASSERT_EQ(nullptr, q.try_pop());

  auto q1 = std::move(q);
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_single)
{
  foo::queue<TypeParam> q;

  foo f;
  q.push(&f);

  auto q1 = std::move(q);
  ASSERT_EQ(&f, q1.try_pop());
  f.hook = nullptr;
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_single_1)
{
  foo::queue<TypeParam> q;

  foo f1, f2;
  q.push(&f1);
  q.push(&f2);
  ASSERT_EQ(&f1, q.try_pop());
  f1.hook = nullptr;

  auto q1 = std::move(q);
  ASSERT_EQ(&f2, q1.try_pop());
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_multiple)
{
  foo::queue<TypeParam> q;

  foo f1, f2;
  q.push(&f1);
  q.push(&f2);

  auto q1 = std::move(q);
  ASSERT_EQ(&f1, q1.try_pop());
  f1.hook = nullptr;
  ASSERT_EQ(&f2, q1.try_pop());
  f2.hook = nullptr;
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_multiple_1)
{
  foo::queue<TypeParam> q;

  foo f1, f2, f3;
  q.push(&f1);
  q.push(&f2);
  q.push(&f3);
  ASSERT_EQ(&f1, q.try_pop());
  f1.hook = nullptr;

  auto q1 = std::move(q);
  ASSERT_EQ(&f2, q1.try_pop());
  f2.hook = nullptr;
  ASSERT_EQ(&f3, q1.try_pop());
  f3.hook = nullptr;
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_assign_empty)
{
  foo::queue<TypeParam> q;
  ASSERT_EQ(nullptr, q.try_pop());

  foo::queue<TypeParam> q1;
  ASSERT_EQ(nullptr, q1.try_pop());

  q1 = std::move(q);
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_assign_empty_1)
{
  foo::queue<TypeParam> q;

  foo f;
  q.push(&f);
  ASSERT_EQ(&f, q.try_pop());
  f.hook = nullptr;
  ASSERT_EQ(nullptr, q.try_pop());

  foo::queue<TypeParam> q1;
  ASSERT_EQ(nullptr, q1.try_pop());

  q1 = std::move(q);
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_assign_single)
{
  foo::queue<TypeParam> q;

  foo f;
  q.push(&f);

  foo::queue<TypeParam> q1;
  q1 = std::move(q);

  ASSERT_EQ(&f, q1.try_pop());
  f.hook = nullptr;
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_assign_single_1)
{
  foo::queue<TypeParam> q;

  foo f1, f2;
  q.push(&f1);
  q.push(&f2);
  ASSERT_EQ(&f1, q.try_pop());
  f1.hook = nullptr;

  foo::queue<TypeParam> q1;
  q1 = std::move(q);

  ASSERT_EQ(&f2, q1.try_pop());
  f2.hook = nullptr;
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_assign_multiple)
{
  foo::queue<TypeParam> q;

  foo f1, f2;
  q.push(&f1);
  q.push(&f2);

  foo::queue<TypeParam> q1;
  q1 = std::move(q);

  ASSERT_EQ(&f1, q1.try_pop());
  f1.hook = nullptr;
  ASSERT_EQ(&f2, q1.try_pop());
  f2.hook = nullptr;
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_assign_multiple_1)
{
  foo::queue<TypeParam> q;

  foo f1, f2, f3;
  q.push(&f1);
  q.push(&f2);
  q.push(&f3);
  ASSERT_EQ(&f1, q.try_pop());
  f1.hook = nullptr;

  foo::queue<TypeParam> q1;
  q1 = std::move(q);

  ASSERT_EQ(&f2, q1.try_pop());
  f2.hook = nullptr;
  ASSERT_EQ(&f3, q1.try_pop());
  f3.hook = nullptr;
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, single_push_pop)
{
  foo::queue<TypeParam> q;

  foo f;
  q.push(&f);
  ASSERT_EQ(&f, q.try_pop());
  f.hook = nullptr;

  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, multiple_push_pop)
{
  foo::queue<TypeParam> q;

  foo f1, f2, f3;
  q.push(&f1);
  q.push(&f2);
  q.push(&f3);

  ASSERT_EQ(&f1, q.try_pop());
  f1.hook = nullptr;
  ASSERT_EQ(&f2, q.try_pop());
  f2.hook = nullptr;
  ASSERT_EQ(&f3, q.try_pop());
  f3.hook = nullptr;

  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, interleaved_push_pop)
{
  foo::queue<TypeParam> q;

  foo f1, f2;
  q.push(&f1);
  q.push(&f2);

  ASSERT_EQ(&f1, q.try_pop());
  f1.hook = nullptr;

  foo f3;
  q.push(&f3);

  ASSERT_EQ(&f2, q.try_pop());
  f2.hook = nullptr;
  q.push(&f2);

  ASSERT_EQ(&f3, q.try_pop());
  f3.hook = nullptr;

  ASSERT_EQ(&f2, q.try_pop());
  f2.hook = nullptr;

  ASSERT_EQ(nullptr, q.try_pop());
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
