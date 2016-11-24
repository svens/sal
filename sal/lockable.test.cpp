#include <sal/lockable.hpp>
#include <sal/common.test.hpp>


namespace {


struct locked_ptr
  : public sal_test::fixture
{
  int data{};
  std::mutex mutex{};
  using ptr = sal::locked_ptr<int, std::mutex>;
};


TEST_F(locked_ptr, ctor_lock)
{
  ptr p(&data, mutex);
  EXPECT_FALSE(mutex.try_lock());
}


TEST_F(locked_ptr, ctor_adopt_lock)
{
  mutex.lock();
  {
    ptr p(&data, mutex, std::adopt_lock);
    EXPECT_TRUE(bool(p));
  }
  EXPECT_TRUE(mutex.try_lock());
  mutex.unlock();
}


TEST_F(locked_ptr, ctor_try_lock)
{
  {
    ptr p(&data, mutex, std::try_to_lock);
    EXPECT_TRUE(bool(p));
  }
  EXPECT_TRUE(mutex.try_lock());
  mutex.unlock();
}


TEST_F(locked_ptr, ctor_try_lock_fail)
{
  mutex.lock();
  {
    ptr p(&data, mutex, std::try_to_lock);
    EXPECT_FALSE(bool(p));
  }
  EXPECT_FALSE(mutex.try_lock());
  mutex.unlock();
}


TEST_F(locked_ptr, ctor_defer_lock)
{
  mutex.lock();
  {
    ptr p(&data, mutex, std::defer_lock);
    EXPECT_TRUE(bool(p));
  }
  EXPECT_FALSE(mutex.try_lock());
  mutex.unlock();
}


TEST_F(locked_ptr, dtor_unlock)
{
  {
    ptr p(&data, mutex);
    EXPECT_FALSE(mutex.try_lock());
  }
  EXPECT_TRUE(mutex.try_lock());
  mutex.unlock();
}


TEST_F(locked_ptr, dtor_no_unlock_unlocked)
{
  {
    ptr p(&data, mutex);
    p.unlock();
    EXPECT_TRUE(mutex.try_lock());
  }
  EXPECT_FALSE(mutex.try_lock());
  mutex.unlock();
}


TEST_F(locked_ptr, move_ctor)
{
  {
    ptr src(&data, mutex);
    EXPECT_TRUE(bool(src));
    EXPECT_EQ(&data, src.get());
    EXPECT_FALSE(mutex.try_lock());

    auto dest = std::move(src);
    EXPECT_TRUE(bool(dest));
    EXPECT_EQ(&data, dest.get());
    EXPECT_FALSE(mutex.try_lock());

    EXPECT_FALSE(bool(src));
    EXPECT_EQ(nullptr, src.get());
    EXPECT_FALSE(mutex.try_lock());
  }
  EXPECT_TRUE(mutex.try_lock());
  mutex.unlock();
}


TEST_F(locked_ptr, move_assign)
{
  {
    ptr src(&data, mutex);
    EXPECT_TRUE(bool(src));
    EXPECT_EQ(&data, src.get());
    EXPECT_FALSE(mutex.try_lock());

    int other_data{};
    std::mutex other_mutex{};
    ptr dest(&other_data, other_mutex);
    EXPECT_FALSE(other_mutex.try_lock());

    dest = std::move(src);
    EXPECT_TRUE(other_mutex.try_lock());
    other_mutex.unlock();

    EXPECT_TRUE(bool(dest));
    EXPECT_EQ(&data, dest.get());
    EXPECT_FALSE(mutex.try_lock());

    EXPECT_FALSE(bool(src));
    EXPECT_EQ(nullptr, src.get());
    EXPECT_FALSE(mutex.try_lock());
  }
  EXPECT_TRUE(mutex.try_lock());
  mutex.unlock();
}


TEST_F(locked_ptr, move_assign_no_unlock_unlocked)
{
  int other_data{};
  std::mutex other_mutex{};
  {
    ptr src(&data, mutex);
    ptr dest(&other_data, other_mutex);

    dest.unlock();
    EXPECT_TRUE(other_mutex.try_lock());

    dest = std::move(src);
    EXPECT_EQ(&data, dest.get());
  }
  EXPECT_FALSE(other_mutex.try_lock());
  other_mutex.unlock();
}


TEST_F(locked_ptr, swap)
{
  int other_data{};
  std::mutex other_mutex{};
  {
    ptr a(&data, mutex);
    EXPECT_EQ(&data, a.get());
    EXPECT_FALSE(mutex.try_lock());

    ptr b(&other_data, other_mutex);
    EXPECT_EQ(&other_data, b.get());
    EXPECT_FALSE(other_mutex.try_lock());

    swap(a, b);
    EXPECT_EQ(&data, b.get());
    EXPECT_FALSE(mutex.try_lock());
    EXPECT_EQ(&other_data, a.get());
    EXPECT_FALSE(other_mutex.try_lock());
  }
  EXPECT_TRUE(mutex.try_lock());
  mutex.unlock();
  EXPECT_TRUE(other_mutex.try_lock());
  other_mutex.unlock();
}


TEST_F(locked_ptr, explicit_unlock)
{
  ptr p(&data, mutex);
  EXPECT_FALSE(mutex.try_lock());

  p.unlock();
  EXPECT_TRUE(mutex.try_lock());
  mutex.unlock();
}


TEST_F(locked_ptr, bool)
{
  ptr p(&data, mutex);
  EXPECT_TRUE(bool(p));

  p.unlock();
  EXPECT_FALSE(bool(p));
}


TEST_F(locked_ptr, operator_arrow)
{
  EXPECT_EQ(0, data);

  sal::locked_ptr<locked_ptr, std::mutex> p(this, mutex);
  EXPECT_EQ(0, p->data);

  p->data = 1;
  EXPECT_EQ(1, data);
}


TEST_F(locked_ptr, operator_deref)
{
  EXPECT_EQ(0, data);

  sal::locked_ptr<int, std::mutex> p(&data, mutex);
  EXPECT_EQ(0, data);

  *p = 1;
  EXPECT_EQ(1, data);
}


} // namespace


namespace {


struct lockable
  : public sal_test::fixture
{
  int data{};
  sal::lockable_t<int> l{data};
};


TEST_F(lockable, lock)
{
  EXPECT_EQ(0, data);

  auto p = l.lock();
  EXPECT_FALSE(bool(l.try_lock()));

  *p = 1;
  EXPECT_EQ(1, data);
}


void lock (const sal::lockable_t<int> &l)
{
  auto p = l.lock();
  EXPECT_FALSE(bool(l.try_lock()));
  EXPECT_EQ(1, *p);
  //COMPILER ERROR: *p = 2;
}


TEST_F(lockable, const_lock)
{
  data = 1;
  lock(l);
  EXPECT_EQ(1, data);
}


TEST_F(lockable, try_lock)
{
  EXPECT_EQ(0, data);

  auto p = l.try_lock();
  EXPECT_FALSE(bool(l.try_lock()));

  *p = 1;
  EXPECT_EQ(1, data);
}


void try_lock (const sal::lockable_t<int> &l)
{
  auto p = l.try_lock();
  EXPECT_FALSE(bool(l.try_lock()));
  EXPECT_EQ(1, *p);
  //COMPILER ERROR: *p = 2;
}


TEST_F(lockable, const_try_lock)
{
  data = 1;
  try_lock(l);
  EXPECT_EQ(1, data);
}


TEST_F(lockable, unlocked)
{
  EXPECT_EQ(0, data);

  auto p = l.unlocked();
  EXPECT_TRUE(bool(l.try_lock()));

  *p = 1;
  EXPECT_EQ(1, data);
}


void unlocked (const sal::lockable_t<int> &l)
{
  auto p = l.unlocked();
  EXPECT_TRUE(bool(l.try_lock()));
  EXPECT_EQ(1, *p);
  //COMPILER ERROR: *p = 2;
}


TEST_F(lockable, const_unlocked)
{
  data = 1;
  unlocked(l);
  EXPECT_EQ(1, data);
}


} // namespace
