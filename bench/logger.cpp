#include <bench/bench.hpp>
#include <sal/logger/logger.hpp>
#include <sal/logger/worker.hpp>
#include <sal/logger/async_worker.hpp>
#include <sal/logger/file_sink.hpp>
#include <algorithm>
#include <thread>
#include <vector>


namespace {

using namespace std::chrono;


// configuration
std::string type = "sync";
size_t lines = 1'000'000;
size_t threads = std::thread::hardware_concurrency();


inline constexpr bool measure_latency ()
{
  return false;
}


template <typename Worker>
void logger_thread (const sal::logger::channel_t<Worker> &channel,
  size_t count, nanoseconds &min, nanoseconds &max, nanoseconds &avg)
{
  time_point<bench::clock_type> start;
  nanoseconds latency;

  for (auto i = 0U;  i != count;  ++i)
  {
    if (measure_latency())
    {
      start = bench::clock_type::now();
    }

    sal_log(channel) << "sal logger message #" << i;

    if (measure_latency())
    {
      latency = duration_cast<nanoseconds>(bench::clock_type::now() - start);
      avg += latency;
      if (latency < min)
      {
        min = latency;
      }
      if (latency > max)
      {
        max = latency;
      }
    }
  }

  if (measure_latency())
  {
    avg /= count;
  }
}


template <typename Worker>
int log_with ()
{
  auto start_time = bench::start();

  {
    Worker worker;

    auto channel = worker.make_channel(type,
      sal::logger::set_channel_sink(
        sal::logger::file(type,
          sal::logger::set_file_dir("bench_logs"),
          sal::logger::set_file_buffer_size_kb(256)
        )
      )
    );

    sal_log(channel) << "lines=" << lines << "; threads=" << threads;

    std::vector<nanoseconds>
      min_latency(threads, nanoseconds::max()),
      max_latency(threads, nanoseconds::min()),
      avg_latency(threads, nanoseconds::zero());

    std::vector<std::thread> logger_threads;
    for (auto i = 0U;  i < threads;  ++i)
    {
      logger_threads.emplace_back(&logger_thread<Worker>,
        std::cref(channel),
        lines/threads,
        std::ref(min_latency[i]),
        std::ref(max_latency[i]),
        std::ref(avg_latency[i])
      );
    }

    for (auto &thread: logger_threads)
    {
      thread.join();
    }

    if (measure_latency())
    {
      auto min = std::min_element(min_latency.begin(), min_latency.end());
      auto max = std::max_element(max_latency.begin(), max_latency.end());
      auto avg = std::max_element(avg_latency.begin(), avg_latency.end());

      std::cout << "latency"
        << ": min=" << min->count() << "ns"
        << "; max=" << max->count() << "ns (" << max->count()/1000 << "us)"
        << "; avg=" << avg->count() << "ns"
        << std::endl;
    }
  }

  bench::stop(start_time, lines);
  return EXIT_SUCCESS;
}


} // namespace


namespace bench {


option_set_t options ()
{
  using namespace sal::program_options;

  option_set_t desc;
  desc
    .add({"t", "type"},
      requires_argument("STRING", type),
      help("worker type (sync | async)")
    )
    .add({"l", "lines"},
      requires_argument("INT", lines),
      help("total number of lines to log")
    )
    .add({"threads"},
      requires_argument("INT", threads),
      help("number of logging threads")
    )
  ;
  return desc;
}


int run (const option_set_t &options, const argument_map_t &arguments)
{
  lines = std::stoul(options.back_or_default("lines", { arguments }));
  threads = std::stoul(options.back_or_default("threads", { arguments }));
  type = options.back_or_default("type", { arguments });

  if (type == "sync")
  {
    return log_with<sal::logger::worker_t>();
  }
  else if (type == "async")
  {
    return log_with<sal::logger::async_worker_t>();
  }

  return usage("unknown worker type '" + type + '\'');
}


} // namespace bench
