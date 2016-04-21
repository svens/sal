#include <sal/spinlock.hpp>
#include <sal/common.test.hpp>
#include <chrono>
#include <mutex>
#include <thread>


TEST(spinlock, single_thread)
{
  sal::spinlock lock;

  lock.lock();
  EXPECT_FALSE(lock.try_lock());
  lock.unlock();

  EXPECT_TRUE(lock.try_lock());
  EXPECT_FALSE(lock.try_lock());
  lock.unlock();
}


TEST(spinlock, two_threads)
{
  using namespace std::chrono_literals;

  sal::spinlock lock;

  // t1.1) keep lock initially locked
  lock.lock();
  bool flag = false;

  auto thread = std::thread([&]
  {
    EXPECT_FALSE(flag);
    EXPECT_FALSE(lock.try_lock());

    // t2.1) spinning here because (t1.1)
    lock.lock();

    // t2.2) owns lock because (t1.3)
    EXPECT_FALSE(flag);

    // t2.3) pretend working little while
    flag = true;
    std::this_thread::sleep_for(2ms);

    // t2.4) stop pretending, give chance to (t1.4)
    lock.unlock();
  });

  // t1.2) still owning lock since (t1.1)
  std::this_thread::sleep_for(1ms);
  EXPECT_FALSE(flag);

  // t1.3) give (t2.1) chance to work
  lock.unlock();
  std::this_thread::sleep_for(1ms);
  ASSERT_FALSE(lock.try_lock());

  // t1.4) spinning here until (t2.3) is pretending
  lock.lock();
  EXPECT_TRUE(flag);
  lock.unlock();

  thread.join();
}


TEST(spinlock, unique_lock_guard)
{
  sal::spinlock lock;

  {
    std::unique_lock<sal::spinlock> guard(lock);
    EXPECT_FALSE(lock.try_lock());

    EXPECT_TRUE(guard.owns_lock());
    guard.unlock();

    EXPECT_TRUE(lock.try_lock());
    EXPECT_FALSE(guard.try_lock());
    lock.unlock();

    EXPECT_TRUE(guard.try_lock());
  }

  EXPECT_TRUE(lock.try_lock());
  lock.unlock();
}
