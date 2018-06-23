#include <bench/bench.hpp>
#include <sal/intrusive_mpsc_queue.hpp>
#include <sal/spinlock.hpp>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>


namespace {

using namespace std::chrono;


// configuration
size_t items = 10'000'000;
size_t producers = 1;
size_t runs = 10;


struct foo_t
{
  bool stop = false;
  sal::intrusive_mpsc_queue_hook_t<foo_t> hook{};
  using queue_t = sal::intrusive_mpsc_queue_t<&foo_t::hook>;
};



milliseconds single_run ()
{
  // preallocate items and create queue
  std::vector<foo_t> nodes(items);
  foo_t::queue_t q;

  std::vector<std::thread> producer_threads;
  std::thread consumer_thread;

  auto start_time = bench::start();

  // start consumer
  consumer_thread = std::thread([&]
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
  });

  // start producers
  std::atomic<size_t> current{};
  for (size_t i = 0;  i != producers;  ++i)
  {
    producer_threads.emplace_back([&]
    {
      for (;;)
      {
        auto x = current++;
        if (x < items)
        {
          q.push(&nodes[x]);
        }
        else
        {
          return;
        }
      }
    });
  }

  // wait producers to finish
  for (auto &thread: producer_threads)
  {
    thread.join();
  }

  // send stop signal to consumer
  foo_t stop_node{true};
  q.push(&stop_node);
  consumer_thread.join();

  return bench::stop(start_time, items);
}


int worker ()
{
  std::vector<milliseconds> times;

  for (size_t i = 0;  i != runs;  ++i)
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


namespace bench {


option_set_t options ()
{
  using namespace sal::program_options;

  option_set_t desc;
  desc
    .add({"i", "items"},
      requires_argument("INT", items),
      help("number of items to push")
    )
    .add({"p", "producers"},
      requires_argument("INT", producers),
      help("number of producer threads")
    )
    .add({"r", "runs"},
      requires_argument("INT", runs),
      help("number of producer threads")
    )
  ;
  return desc;
}


int run (const option_set_t &options, const argument_map_t &arguments)
{
  items = std::stoul(options.back_or_default("items", { arguments }));
  producers = std::stoul(options.back_or_default("producers", { arguments }));
  runs = std::stoul(options.back_or_default("runs", { arguments }));
  return worker();
}


} // namespace bench
