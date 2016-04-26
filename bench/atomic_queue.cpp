#include <bench/bench.hpp>
#include <sal/atomic_queue.hpp>
#include <iostream>
#include <set>
#include <thread>
#include <vector>


namespace {


using namespace std::chrono;


// configuration
size_t count = 10'000'000;
size_t producers = 1, consumers = 1;
size_t run = 10;
std::string type = "mpmc";

const std::set<std::string> allowed_types =
{
  "mpmc", "mpsc", "spmc", "spsc"
};


int usage (const std::string message="")
{
  if (!message.empty())
  {
    std::cerr << message << '\n' << std::endl;
  }

  std::cerr << "atomic_queue:"
    << "\n  --help         this page"
    << "\n  --consumers=N  number of consumer threads (default: " << consumers << ')'
    << "\n  --count=int    number of items to push (default: " << count << ')'
    << "\n  --producers=N  number of producer threads (default: " << producers << ')'
    << "\n  --type=Type    queue type (default: " << type << ')'
    << "\n                 possible values: mpmc, mpsc, spmc, spsc"
    << std::endl;

  return EXIT_FAILURE;
}


struct foo
{
  bool stop = false;
  sal::atomic_queue_hook<foo> hook{};

  using array = std::vector<foo>;

  template <typename UsePolicy>
  using queue = sal::atomic_queue<foo, &foo::hook, UsePolicy>;
};


template <typename QueueType>
milliseconds single_run ()
{
  // preallocate items and create queue
  foo::array array(count);
  foo::queue<QueueType> queue;

  std::vector<std::thread> consumer_threads, producer_threads;
  auto start_time = bench::start();

  // start consumers
  for (size_t i = 0;  i != consumers;  ++i)
  {
    consumer_threads.emplace_back(
      [&]
      {
        for (;;)
        {
          auto node = queue.try_pop();
          if (!node)
          {
            std::this_thread::sleep_for(1us);
            continue;
          }
          if (node->stop)
          {
            break;
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
            queue.push(&array[x]);
          }
          else
          {
            return;
          }
        }
      }
    );
  }

  size_t percent = 0;
  while (bench::in_progress(current, count, percent))
  {
    ;
  }

  // wait producers to finish
  for (auto &thread: producer_threads)
  {
    thread.join();
  }

  // send stop signal to consumers
  foo::array stop_array(consumers);
  for (size_t i = 0;  i != consumers;  ++i)
  {
    stop_array[i].stop = true;
    queue.push(&stop_array[i]);
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
  milliseconds min(milliseconds::max()), max(milliseconds::min());

  for (size_t i = 0;  i != run;  ++i)
  {
    milliseconds current{};

    if (type == "mpmc")
    {
      current = single_run<sal::mpmc>();
    }
    else if (type == "mpsc")
    {
      current = single_run<sal::mpsc>();
    }
    else if (type == "spmc")
    {
      current = single_run<sal::spmc>();
    }
    else if (type == "spsc")
    {
      current = single_run<sal::spsc>();
    }

    if (current < min)
    {
      min = current;
    }
    if (current > max)
    {
      max = current;
    }
  }

  std::cout << "\nmin " << min.count() << "ms"
    ", max " << max.count() << "ms"
    << std::endl;

  return EXIT_SUCCESS;
}


} // namespace


int bench::atomic_queue (const arg_list &args)
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
    else if (arg.find("--type=") != arg.npos)
    {
      auto tmp = arg.substr(sizeof("--type=") - 1);
      if (!allowed_types.count(tmp))
      {
        return usage("unknown type: " + tmp);
      }
      type = tmp;
    }
    else
    {
      return usage("unknown argument: " + arg);
    }
  }

  return worker();
}
