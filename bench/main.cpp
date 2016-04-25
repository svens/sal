#include <bench/bench.hpp>
#include <iomanip>
#include <iostream>


bench::func_list bench_func =
{
  { "atomic_queue", &bench::atomic_queue },
  { "c_str", &bench::c_str },
  { "spinlock", &bench::spinlock },
};


int usage (const char *arg0)
{
  std::string exe = arg0;
  auto filename_begin = exe.find_last_of("/\\");
  if (filename_begin != exe.npos)
  {
    exe.erase(exe.begin(), exe.begin() + filename_begin + 1);
  }

  std::cerr << "usage:"
    << "\n  " << exe << " module [options]"
    << "\n\nmodules:"
    << std::endl;

  for (const auto &f: bench_func)
  {
    std::cerr << "  " << f.first << std::endl;
  }

  std::cerr << "\nFor more information, run"
    " \"" << exe << " module --help\"\n"
    << std::endl;

  return EXIT_FAILURE;
}


int main (int argc, const char *argv[])
{
  if (argc < 2)
  {
    return usage(argv[0]);
  }

  auto f = bench_func.find(argv[1]);
  if (f == bench_func.end())
  {
    return usage(argv[0]);
  }

  return (f->second)({argv + 2, argv + argc});
}


namespace bench {


time_point starting ()
{
  return clock_type::now();
}


void stopped (time_point start_time, size_t count)
{
  using namespace std::chrono;

  auto delay = clock_type::now() - start_time;
  auto msec = duration_cast<milliseconds>(delay).count();
  if (!msec)
  {
    msec = 1;
  }

  std::cout << '\n' << msec << " msec"
    << ", " << count/msec << " count/msec"
    << std::endl;
}


bool in_progress (size_t current, size_t count, size_t &percent)
{
  if (current > count)
  {
    return false;
  }

  auto new_percent = current * 100 / count;
  if (current == 1 || percent != new_percent)
  {
    percent = new_percent;
    std::cout << "\r[" << std::setw(3) << percent << "%] " << std::flush;
  }

  return true;
}


} // namespace bench
