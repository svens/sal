#include <sal/intrusive_stack.hpp>
#include <sal/common.test.hpp>
#include <array>


namespace {


struct intrusive_stack
  : public sal_test::fixture
{
  struct foo_t
  {
    sal::intrusive_stack_hook_t<foo_t> hook;
  };
  using stack_t = sal::intrusive_stack_t<foo_t, &foo_t::hook>;
  stack_t stack{};
};


TEST_F(intrusive_stack, ctor)
{
  EXPECT_EQ(nullptr, stack.try_pop());
  EXPECT_TRUE(stack.empty());
}


TEST_F(intrusive_stack, move_ctor_empty)
{
  ASSERT_EQ(nullptr, stack.try_pop());

  auto s = std::move(stack);
  ASSERT_EQ(nullptr, s.try_pop());
  EXPECT_TRUE(s.empty());
}


TEST_F(intrusive_stack, move_ctor_empty_1)
{
  foo_t foo;
  stack.push(&foo);
  EXPECT_FALSE(stack.empty());
  ASSERT_EQ(&foo, stack.try_pop());
  EXPECT_TRUE(stack.empty());

  auto s = std::move(stack);
  ASSERT_EQ(nullptr, s.try_pop());
  EXPECT_TRUE(s.empty());
}


TEST_F(intrusive_stack, move_ctor_single)
{
  foo_t f;
  stack.push(&f);

  auto s = std::move(stack);
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f, s.try_pop());
  ASSERT_TRUE(s.empty());

  ASSERT_EQ(nullptr, s.try_pop());
  ASSERT_TRUE(s.empty());
}


TEST_F(intrusive_stack, move_ctor_single_1)
{
  foo_t f1, f2;
  stack.push(&f1);
  stack.push(&f2);
  ASSERT_FALSE(stack.empty());

  ASSERT_EQ(&f2, stack.try_pop());
  ASSERT_FALSE(stack.empty());

  auto s = std::move(stack);
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f1, s.try_pop());
  ASSERT_TRUE(s.empty());

  ASSERT_EQ(nullptr, s.try_pop());
  ASSERT_TRUE(s.empty());
}


TEST_F(intrusive_stack, move_ctor_multiple)
{
  foo_t f1, f2;
  stack.push(&f1);
  stack.push(&f2);

  auto s = std::move(stack);
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f2, s.try_pop());
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f1, s.try_pop());
  ASSERT_TRUE(s.empty());

  ASSERT_EQ(nullptr, s.try_pop());
  ASSERT_TRUE(s.empty());
}


TEST_F(intrusive_stack, move_ctor_multiple_1)
{
  foo_t f1, f2, f3;
  stack.push(&f1);
  stack.push(&f2);
  stack.push(&f3);
  ASSERT_EQ(&f3, stack.try_pop());

  auto s = std::move(stack);
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f2, s.try_pop());
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f1, s.try_pop());
  ASSERT_TRUE(s.empty());

  ASSERT_EQ(nullptr, s.try_pop());
  ASSERT_TRUE(s.empty());
}


TEST_F(intrusive_stack, move_assign_empty)
{
  stack_t s;
  s = std::move(stack);
  ASSERT_EQ(nullptr, s.try_pop());
  ASSERT_TRUE(s.empty());
}


TEST_F(intrusive_stack, move_assign_empty_1)
{
  stack_t s;

  foo_t f1;
  stack.push(&f1);
  ASSERT_EQ(&f1, stack.try_pop());

  s = std::move(stack);
  ASSERT_EQ(nullptr, s.try_pop());
  ASSERT_TRUE(s.empty());
}


TEST_F(intrusive_stack, move_assign_single)
{
  stack_t s;

  foo_t f1;
  stack.push(&f1);

  s = std::move(stack);
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f1, s.try_pop());
  ASSERT_TRUE(s.empty());

  ASSERT_EQ(nullptr, s.try_pop());
  ASSERT_TRUE(s.empty());
}


TEST_F(intrusive_stack, move_assign_single_1)
{
  stack_t s;

  foo_t f1, f2;
  stack.push(&f1);
  stack.push(&f2);
  ASSERT_EQ(&f2, stack.try_pop());

  s = std::move(stack);
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f1, s.try_pop());
  ASSERT_TRUE(s.empty());

  ASSERT_EQ(nullptr, s.try_pop());
  ASSERT_TRUE(s.empty());
}


TEST_F(intrusive_stack, move_assign_multiple)
{
  stack_t s;

  foo_t f1, f2;
  stack.push(&f1);
  stack.push(&f2);

  s = std::move(stack);
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f2, s.try_pop());
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f1, s.try_pop());
  ASSERT_TRUE(s.empty());

  ASSERT_EQ(nullptr, s.try_pop());
  ASSERT_TRUE(s.empty());
}


TEST_F(intrusive_stack, move_assign_multiple_1)
{
  stack_t s;

  foo_t f1, f2, f3;
  stack.push(&f1);
  stack.push(&f2);
  stack.push(&f3);
  ASSERT_EQ(&f3, stack.try_pop());

  s = std::move(stack);
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f2, s.try_pop());
  ASSERT_FALSE(s.empty());

  ASSERT_EQ(&f1, s.try_pop());
  ASSERT_TRUE(s.empty());

  ASSERT_EQ(nullptr, s.try_pop());
  ASSERT_TRUE(s.empty());
}


TEST_F(intrusive_stack, single_push_pop)
{
  foo_t f;
  ASSERT_TRUE(stack.empty());

  stack.push(&f);
  ASSERT_FALSE(stack.empty());

  ASSERT_EQ(&f, stack.try_pop());
  ASSERT_TRUE(stack.empty());

  ASSERT_EQ(nullptr, stack.try_pop());
  ASSERT_TRUE(stack.empty());
}


TEST_F(intrusive_stack, multiple_push_pop)
{
  foo_t f1, f2, f3;
  ASSERT_TRUE(stack.empty());
  stack.push(&f1);
  stack.push(&f2);
  stack.push(&f3);
  ASSERT_FALSE(stack.empty());

  ASSERT_EQ(&f3, stack.try_pop());
  ASSERT_FALSE(stack.empty());

  ASSERT_EQ(&f2, stack.try_pop());
  ASSERT_FALSE(stack.empty());

  ASSERT_EQ(&f1, stack.try_pop());
  ASSERT_TRUE(stack.empty());

  ASSERT_EQ(nullptr, stack.try_pop());
  ASSERT_TRUE(stack.empty());
}


TEST_F(intrusive_stack, interleaved_push_pop)
{
  // push 1, 2
  foo_t f1, f2;
  stack.push(&f1);
  stack.push(&f2);
  ASSERT_FALSE(stack.empty());

  // pop 2
  ASSERT_EQ(&f2, stack.try_pop());
  ASSERT_FALSE(stack.empty());

  // push 3
  foo_t f3;
  stack.push(&f3);
  ASSERT_FALSE(stack.empty());

  // pop 3, push 3
  ASSERT_EQ(&f3, stack.try_pop());
  stack.push(&f3);
  ASSERT_FALSE(stack.empty());

  // pop 3
  ASSERT_EQ(&f3, stack.try_pop());
  ASSERT_FALSE(stack.empty());

  // pop 1
  ASSERT_EQ(&f1, stack.try_pop());
  ASSERT_TRUE(stack.empty());

  // pop nil
  ASSERT_EQ(nullptr, stack.try_pop());
  ASSERT_TRUE(stack.empty());
}


} // namespace
