#include <bench/bench.hpp>
#include <sal/concurrent_queue.hpp>
#include <algorithm>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>


namespace {


using namespace std::chrono;


// configuration
size_t count = 10'000'000;
size_t producers = 1, consumers = 1;
size_t run = 10;


int usage (const std::string message="")
{
  if (!message.empty())
  {
    std::cerr << message << '\n' << std::endl;
  }

  std::cerr << "concurrent_queue:"
    << "\n  --help         this page"
    << "\n  --count=int    number of items to push (default: " << count << ')'
    << "\n  --consumers=N  number of consumer threads (default: " << consumers << ')'
    << "\n  --producers=N  number of producer threads (default: " << producers << ')'
    << std::endl;

  return EXIT_FAILURE;
}


struct foo
{
  sal::concurrent_queue_hook hook{};
  bool stop = false;
};


milliseconds single_run ()
{
  using queue = sal::concurrent_queue<foo, &foo::hook>;

  // preallocate items and create queue
  std::vector<foo> nodes(count);
  queue q;

  std::vector<std::thread> consumer_threads, producer_threads;
  auto start_time = bench::start();

  // start consumers
  for (size_t i = 0;  i != consumers;  ++i)
  {
    consumer_threads.emplace_back(
      [&]
      {
        for (size_t i = 0;  /**/;  ++i)
        {
          if (auto n = q.try_pop())
          {
            if (n->stop)
            {
              break;
            }
          }
          else
          {
            sal::adaptive_spin<100>(i);
          }
        }
      }
    );
  }

  // start producers
  std::atomic<size_t> current{};
  for (size_t i = 0;  i != producers;  ++i)
  {
    producer_threads.emplace_back(
      [&, i]
      {
        for (;;)
        {
          auto x = current++;
          if (x < count)
          {
            q.push(&nodes[x]);
          }
          else
          {
            return;
          }
        }
      }
    );
  }

  // wait producers to finish
  for (auto &thread: producer_threads)
  {
    thread.join();
  }

  // send stop signal to consumers
  std::vector<foo> stop_nodes(consumers);
  for (size_t i = 0;  i != consumers;  ++i)
  {
    stop_nodes[i].stop = true;
    q.push(&stop_nodes[i]);
  }

  // wait consumers to finish
  for (auto &thread: consumer_threads)
  {
    thread.join();
  }

  return bench::stop(start_time, count);
}


int worker ()
{
  std::vector<milliseconds> times;

  for (size_t i = 0;  i != run;  ++i)
  {
    times.emplace_back(single_run());
  }

  std::sort(times.begin(), times.end());

  std::cout << "\nmin " << times.front().count() << "ms"
    ", max " << times.back().count() << "ms"
    ", median " << times[times.size()/2].count() << "ms"
    << std::endl;

  return EXIT_SUCCESS;
}


} // namespace


int bench::concurrent_queue (const arg_list &args)
{
  for (auto &arg: args)
  {
    if (arg == "--help")
    {
      return usage();
    }
    else if (arg.find("--consumers=") != arg.npos)
    {
      consumers = std::stoul(arg.substr(sizeof("--consumers=") - 1));
    }
    else if (arg.find("--count=") != arg.npos)
    {
      count = std::stoul(arg.substr(sizeof("--count=") - 1));
    }
    else if (arg.find("--producers=") != arg.npos)
    {
      producers = std::stoul(arg.substr(sizeof("--producers=") - 1));
    }
    else if (arg.find("--run=") != arg.npos)
    {
      run = std::stoul(arg.substr(sizeof("--run=") - 1));
    }
    else
    {
      return usage("unknown argument: " + arg);
    }
  }

  return worker();
}
