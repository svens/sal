#include <sal/concurrent_queue.hpp>
#include <sal/common.test.hpp>


//namespace {


template <typename ConcurrentUsage>
struct test
  : public sal_test::with_type<ConcurrentUsage>
{
  struct foo
  {
    sal::queue_hook<ConcurrentUsage::value> hook;
  };
  using queue = sal::queue<ConcurrentUsage::value, foo, &foo::hook>;
};


using types = testing::Types<
  std::integral_constant<sal::concurrent_usage, sal::concurrent_usage::none>,
  std::integral_constant<sal::concurrent_usage, sal::concurrent_usage::mpmc>,
  std::integral_constant<sal::concurrent_usage, sal::concurrent_usage::mpsc>,
  std::integral_constant<sal::concurrent_usage, sal::concurrent_usage::spmc>,
  std::integral_constant<sal::concurrent_usage, sal::concurrent_usage::spsc>
>;


TYPED_TEST_CASE_P(test);


TYPED_TEST_P(test, ctor)
{
  typename TestFixture::queue q;
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_ctor_empty)
{
  typename TestFixture::queue q;
  ASSERT_EQ(nullptr, q.try_pop());

  auto q1 = std::move(q);
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_empty_1)
{
  typename TestFixture::queue q;

  typename TestFixture::foo f;
  q.push(&f);
  ASSERT_EQ(&f, q.try_pop());

  auto q1 = std::move(q);
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_single)
{
  typename TestFixture::queue q;

  typename TestFixture::foo f;
  q.push(&f);

  auto q1 = std::move(q);
  ASSERT_EQ(&f, q1.try_pop());
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_single_1)
{
  typename TestFixture::queue q;

  typename TestFixture::foo f1, f2;
  q.push(&f1);
  q.push(&f2);
  ASSERT_EQ(&f1, q.try_pop());

  auto q1 = std::move(q);
  ASSERT_EQ(&f2, q1.try_pop());
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_multiple)
{
  typename TestFixture::queue q;

  typename TestFixture::foo f1, f2;
  q.push(&f1);
  q.push(&f2);

  auto q1 = std::move(q);
  ASSERT_EQ(&f1, q1.try_pop());
  ASSERT_EQ(&f2, q1.try_pop());
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_ctor_multiple_1)
{
  typename TestFixture::queue q;

  typename TestFixture::foo f1, f2, f3;
  q.push(&f1);
  q.push(&f2);
  q.push(&f3);
  ASSERT_EQ(&f1, q.try_pop());

  auto q1 = std::move(q);
  ASSERT_EQ(&f2, q1.try_pop());
  ASSERT_EQ(&f3, q1.try_pop());
  ASSERT_EQ(nullptr, q1.try_pop());
}


TYPED_TEST_P(test, move_assign_empty)
{
  typename TestFixture::queue q1, q2;
  q2 = std::move(q1);
  ASSERT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, move_assign_empty_1)
{
  typename TestFixture::queue q1, q2;

  typename TestFixture::foo f1;
  q1.push(&f1);
  ASSERT_EQ(&f1, q1.try_pop());

  q2 = std::move(q1);
  ASSERT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, move_assign_single)
{
  typename TestFixture::queue q1, q2;

  typename TestFixture::foo f1;
  q1.push(&f1);

  q2 = std::move(q1);
  ASSERT_EQ(&f1, q2.try_pop());
  ASSERT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, move_assign_single_1)
{
  typename TestFixture::queue q1, q2;

  typename TestFixture::foo f1, f2;
  q1.push(&f1);
  q1.push(&f2);
  ASSERT_EQ(&f1, q1.try_pop());

  q2 = std::move(q1);
  ASSERT_EQ(&f2, q2.try_pop());
  ASSERT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, move_assign_multiple)
{
  typename TestFixture::queue q1, q2;

  typename TestFixture::foo f1, f2;
  q1.push(&f1);
  q1.push(&f2);

  q2 = std::move(q1);
  ASSERT_EQ(&f1, q2.try_pop());
  ASSERT_EQ(&f2, q2.try_pop());
  ASSERT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, move_assign_multiple_1)
{
  typename TestFixture::queue q1, q2;

  typename TestFixture::foo f1, f2, f3;
  q1.push(&f1);
  q1.push(&f2);
  q1.push(&f3);
  ASSERT_EQ(&f1, q1.try_pop());

  q2 = std::move(q1);
  ASSERT_EQ(&f2, q2.try_pop());
  ASSERT_EQ(&f3, q2.try_pop());
  ASSERT_EQ(nullptr, q2.try_pop());
}


TYPED_TEST_P(test, single_push_pop)
{
  typename TestFixture::queue q;

  typename TestFixture::foo f;
  q.push(&f);
  ASSERT_EQ(&f, q.try_pop());

  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, multiple_push_pop)
{
  typename TestFixture::queue q;

  typename TestFixture::foo f1, f2, f3;
  q.push(&f1);
  q.push(&f2);
  q.push(&f3);

  ASSERT_EQ(&f1, q.try_pop());
  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_EQ(&f3, q.try_pop());
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, interleaved_push_pop)
{
  typename TestFixture::queue q;

  // push 1, 2
  typename TestFixture::foo f1, f2;
  q.push(&f1);
  q.push(&f2);

  // pop 1
  ASSERT_EQ(&f1, q.try_pop());

  // push 3
  typename TestFixture::foo f3;
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


//} // namespace
