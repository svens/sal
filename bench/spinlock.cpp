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


int usage (const std::string message="")
{
  if (!message.empty())
  {
    std::cerr << message << '\n' << std::endl;
  }

  std::cerr << "spinlock:"
    << "\n  --help        this page"
    << "\n  --count=int   number of iteration (default: " << count << ')'
    << "\n  --lock=Lock   lock type (default: " << lock << ')'
    << "\n                possible values: spinlock, mutex"
    << "\n  --spin=int    number of spins before yield (default: " << spin << ')'
    << "\n  --threads=int number of threads (default: " << threads << ')'
    << std::endl;

  return EXIT_FAILURE;
}


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
  auto start_time = bench::starting();
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

  bench::stopped(start_time, count);

  return EXIT_SUCCESS;
}


} // namespace


int bench::spinlock (const arg_list &args)
{
  for (auto &arg: args)
  {
    if (arg == "--help")
    {
      return usage();
    }
    else if (arg.find("--count=") != arg.npos)
    {
      count = std::stoul(arg.substr(sizeof("--count=") - 1));
    }
    else if (arg.find("--spin=") != arg.npos)
    {
      spin = std::stoul(arg.substr(sizeof("--spin=") - 1));
    }
    else if (arg.find("--threads=") != arg.npos)
    {
      threads = std::stoul(arg.substr(sizeof("--threads=") - 1));
    }
    else if (arg.find("--lock=") != arg.npos)
    {
      lock = arg.substr(sizeof("--lock=") - 1);
    }
    else
    {
      return usage("unknown argument: " + arg);
    }
  }

  if (lock == "spinlock")
  {
    sal::spinlock mutex;
    return worker(mutex);
  }
  else if (lock == "mutex")
  {
    std::mutex mutex;
    return worker(mutex);
  }

  return usage("unknown lock '" + lock + '\'');
}
