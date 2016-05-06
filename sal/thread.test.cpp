#include <sal/thread.hpp>
#include <sal/common.test.hpp>
#include <mutex>
#include <set>
#include <thread>


namespace {


TEST(thread, get_id)
{
  std::mutex mutex;
  std::set<sal::thread_id> thread_ids;

  thread_ids.insert(sal::this_thread::get_id());
  for (auto i = 0;  i < 10;  ++i)
  {
    std::thread(
      [&]
      {
        // get id and check that next call returns same
        auto id = sal::this_thread::get_id();
        EXPECT_EQ(id, sal::this_thread::get_id());

        // insert into set and check it is unique
        std::unique_lock<std::mutex> lock(mutex);
        EXPECT_EQ(true, thread_ids.insert(id).second);
      }
    ).join();
  }

  EXPECT_EQ(11U, thread_ids.size());
}


} // namespace
