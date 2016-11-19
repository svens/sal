#include <sal/intrusive_queue.hpp>
#include <sal/common.test.hpp>


namespace {


template <typename QueueHook>
struct test
  : public sal_test::with_type<QueueHook>
{
  struct foo_t
  {
    typename QueueHook::intrusive_queue_hook_t hook;
  };
  using queue_t = sal::intrusive_queue_t<foo_t, QueueHook, &foo_t::hook>;
  queue_t queue{};
};


using types = testing::Types<
  //sal::no_sync_t
  //sal::spsc_sync_t
  sal::mpsc_sync_t
>;


TYPED_TEST_CASE_P(test);


TYPED_TEST_P(test, ctor)
{
  ASSERT_EQ(nullptr, this->queue.try_pop());
}


TYPED_TEST_P(test, move_ctor_empty)
{
  ASSERT_EQ(nullptr, this->queue.try_pop());

  auto q = std::move(this->queue);
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_ctor_empty_1)
{
  typename TestFixture::foo_t f;
  this->queue.push(&f);
  ASSERT_EQ(&f, this->queue.try_pop());

  auto q = std::move(this->queue);
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_ctor_single)
{
  typename TestFixture::foo_t f;
  this->queue.push(&f);

  auto q = std::move(this->queue);
  ASSERT_EQ(&f, q.try_pop());
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_ctor_single_1)
{
  typename TestFixture::foo_t f1, f2;
  this->queue.push(&f1);
  this->queue.push(&f2);
  ASSERT_EQ(&f1, this->queue.try_pop());

  auto q = std::move(this->queue);
  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_ctor_multiple)
{
  typename TestFixture::foo_t f1, f2;
  this->queue.push(&f1);
  this->queue.push(&f2);

  auto q = std::move(this->queue);
  ASSERT_EQ(&f1, q.try_pop());
  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_ctor_multiple_1)
{
  typename TestFixture::foo_t f1, f2, f3;
  this->queue.push(&f1);
  this->queue.push(&f2);
  this->queue.push(&f3);
  ASSERT_EQ(&f1, this->queue.try_pop());

  auto q = std::move(this->queue);
  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_EQ(&f3, q.try_pop());
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_assign_empty)
{
  typename TestFixture::queue_t q;
  q = std::move(this->queue);
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_assign_empty_1)
{
  typename TestFixture::queue_t q;

  typename TestFixture::foo_t f1;
  this->queue.push(&f1);
  ASSERT_EQ(&f1, this->queue.try_pop());

  q = std::move(this->queue);
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_assign_single)
{
  typename TestFixture::queue_t q;

  typename TestFixture::foo_t f1;
  this->queue.push(&f1);

  q = std::move(this->queue);
  ASSERT_EQ(&f1, q.try_pop());
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_assign_single_1)
{
  typename TestFixture::queue_t q;

  typename TestFixture::foo_t f1, f2;
  this->queue.push(&f1);
  this->queue.push(&f2);
  ASSERT_EQ(&f1, this->queue.try_pop());

  q = std::move(this->queue);
  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_assign_multiple)
{
  typename TestFixture::queue_t q;

  typename TestFixture::foo_t f1, f2;
  this->queue.push(&f1);
  this->queue.push(&f2);

  q = std::move(this->queue);
  ASSERT_EQ(&f1, q.try_pop());
  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, move_assign_multiple_1)
{
  typename TestFixture::queue_t q;

  typename TestFixture::foo_t f1, f2, f3;
  this->queue.push(&f1);
  this->queue.push(&f2);
  this->queue.push(&f3);
  ASSERT_EQ(&f1, this->queue.try_pop());

  q = std::move(this->queue);
  ASSERT_EQ(&f2, q.try_pop());
  ASSERT_EQ(&f3, q.try_pop());
  ASSERT_EQ(nullptr, q.try_pop());
}


TYPED_TEST_P(test, single_push_pop)
{
  typename TestFixture::foo_t f;
  this->queue.push(&f);
  ASSERT_EQ(&f, this->queue.try_pop());

  ASSERT_EQ(nullptr, this->queue.try_pop());
}


TYPED_TEST_P(test, multiple_push_pop)
{
  typename TestFixture::foo_t f1, f2, f3;
  this->queue.push(&f1);
  this->queue.push(&f2);
  this->queue.push(&f3);

  ASSERT_EQ(&f1, this->queue.try_pop());
  ASSERT_EQ(&f2, this->queue.try_pop());
  ASSERT_EQ(&f3, this->queue.try_pop());
  ASSERT_EQ(nullptr, this->queue.try_pop());
}


TYPED_TEST_P(test, interleaved_push_pop)
{
  // push 1, 2
  typename TestFixture::foo_t f1, f2;
  this->queue.push(&f1);
  this->queue.push(&f2);

  // pop 1
  ASSERT_EQ(&f1, this->queue.try_pop());

  // push 3
  typename TestFixture::foo_t f3;
  this->queue.push(&f3);

  // pop 2, push 2
  ASSERT_EQ(&f2, this->queue.try_pop());
  this->queue.push(&f2);

  // pop 3
  ASSERT_EQ(&f3, this->queue.try_pop());

  // pop 2
  ASSERT_EQ(&f2, this->queue.try_pop());

  // pop nil
  ASSERT_EQ(nullptr, this->queue.try_pop());
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


INSTANTIATE_TYPED_TEST_CASE_P(intrusive_queue, test, types);


} // namespace
