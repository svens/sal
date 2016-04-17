#include <bench/bench.hpp>
#include <iomanip>
#include <iostream>


bench::func_list bench_func =
{
  { "view", &bench::view },
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
    std::cerr << " " << f.first << std::endl;
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


bool in_progress (size_t &current, size_t total, size_t &percent)
{
  if (current == total)
  {
    return false;
  }

  current++;
  auto new_percent = current * 100 / total;
  if (current == 1 || percent != new_percent)
  {
    percent = new_percent;
    std::cout << "\r[" << std::setw(3) << percent << "%] " << std::flush;
  }

  return true;
}


} // namespace bench
