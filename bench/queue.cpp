#include <bench/bench.hpp>
#include <sal/queue.hpp>
#include <sal/spinlock.hpp>
#include <algorithm>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
#include <set>


namespace {

using namespace std::chrono;


// configuration
size_t run = 10;
size_t count = 10'000'000;
size_t producers = 1, consumers = 1;
std::string type = "mpsc";

const std::set<std::string> valid_types{
  "mpsc",
  "spsc",
};


template <typename QueueHook>
struct foo
{
  bool stop = false;
  QueueHook hook{};
  using queue = sal::queue_t<foo, QueueHook, &foo::hook>;
};



template <typename QueueHook>
milliseconds single_run ()
{
  // preallocate items and create queue
  std::vector<foo<QueueHook>> nodes(count);
  typename foo<QueueHook>::queue q;

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
  std::vector<foo<QueueHook>> stop_nodes(consumers);
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
    if (type == "mpsc")
    {
      times.emplace_back(single_run<sal::mpsc_t>());
    }
    else if (type == "spsc")
    {
      times.emplace_back(single_run<sal::spsc_t>());
    }
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
    .add({"consumers"},
      requires_argument("INT", consumers),
      help("number of consumer threads")
    )
    .add({"producers"},
      requires_argument("INT", producers),
      help("number of producer threads")
    )
    .add({"t", "type"},
      requires_argument("STRING", type),
      help("queue concurrency pattern type (mpsc | spsc)")
    )
  ;
  return desc;
}


int run (const option_set_t &options, const argument_map_t &arguments)
{
  count = std::stoul(options.back_or_default("count", { arguments }));
  consumers = std::stoul(options.back_or_default("consumers", { arguments }));
  producers = std::stoul(options.back_or_default("producers", { arguments }));
  type = options.back_or_default("type", { arguments });

  if (!valid_types.count(type))
  {
    return usage("unknown type '" + type + '\'');
  }

  return worker();
}


} // namespace bench
