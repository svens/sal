#include <bench/bench.hpp>
#include <sal/spinlock.hpp>
#include <iostream>
#include <mutex>
#include <thread>


namespace {


// configuration
std::string lock = "spinlock";
size_t count = 10'000'000;
size_t spin = 100;
size_t threads = 2;


template <typename Mutex>
int worker (Mutex &mutex)
{
  size_t current = 0, percent = 0;

  // lock mutex before starting workers
  // keep them blocked until we start bench loop
  mutex.lock();

  std::vector<std::thread> workers;
  while (workers.size() != threads)
  {
    workers.emplace_back(
      [&mutex, &current]
      {
        for (;;)
        {
          std::lock_guard<Mutex> guard(mutex);
          if (++current > count)
          {
            return;
          }
        }
      }
    );
  }

  // all workers ready, start timer and let them go
  auto start_time = bench::start();
  mutex.unlock();

  while (bench::in_progress(current, count, percent))
  {
    ;
  }

  // stop all threads
  for (auto &worker: workers)
  {
    worker.join();
  }

  bench::stop(start_time, count);

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
      help("number of iterations")
    )
    .add({"s", "spin"},
      requires_argument("INT", spin),
      help("number of spins before yield")
    )
    .add({"t", "threads"},
      requires_argument("INT", threads),
      help("number of threads")
    )
    .add({"l", "lock"},
      requires_argument("STRING", lock),
      help("lock type (spinlock | mutex)")
    )
  ;
  return desc;
}


int run (const option_set_t &options, const argument_map_t &arguments)
{
  count = std::stoul(options.back_or_default("count", { arguments }));
  spin = std::stoul(options.back_or_default("spin", { arguments }));
  threads = std::stoul(options.back_or_default("threads", { arguments }));
  lock = options.back_or_default("lock", { arguments });

  if (lock == "spinlock")
  {
    sal::spinlock_t mutex;
    return worker(mutex);
  }
  else if (lock == "mutex")
  {
    std::mutex mutex;
    return worker(mutex);
  }

  return usage("unknown lock '" + lock + '\'');
}


} // namespace bench
