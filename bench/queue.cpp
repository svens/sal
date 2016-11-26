#include <bench/bench.hpp>
#include <sal/queue.hpp>
#include <sal/spinlock.hpp>
#include <algorithm>
#include <iostream>
#include <thread>


namespace {

using namespace std::chrono;


// configuration
size_t run = 10;
int count = 10'000'000;


template <typename SyncPolicy>
milliseconds single_run ()
{
  sal::queue_t<int, sal::spsc_sync_t> queue;

  auto start_time = bench::start();

  // consumer
  auto consumer = std::thread([&]
  {
    for (int i = 0;  i != -1;  /**/)
    {
      if (!queue.try_pop(&i))
      {
        std::this_thread::yield();
      }
    }
  });

  // producer
  for (int i = 0;  i != count;  ++i)
  {
    queue.push(i);
  }

  // stop
  queue.push(-1);
  consumer.join();

  return bench::stop(start_time, count);
}


int worker ()
{
  std::vector<milliseconds> times;

  for (size_t i = 0;  i != run;  ++i)
  {
    times.emplace_back(single_run<sal::spsc_sync_t>());
  }

  std::sort(times.begin(), times.end());

  std::cout << "\nmin " << times.front().count() << "ms"
    ", max " << times.back().count() << "ms"
    ", median " << times[times.size()/2].count() << "ms"
    << std::endl;

  return EXIT_SUCCESS;
}


} // namespace


namespace bench {


option_set_t options ()
{
  using namespace sal::program_options;

  option_set_t desc;
  desc
    .add({"c", "count"},
      requires_argument("INT", count),
      help("number of items to push")
    )
  ;
  return desc;
}


int run (const option_set_t &options, const argument_map_t &arguments)
{
  count = std::stoul(options.back_or_default("count", { arguments }));
  return worker();
}


} // namespace bench
