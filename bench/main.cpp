#include "bench.hpp"
#include <sal/config.hpp>
#include <iostream>


bench::func_list bench_func =
{
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
  std::cout << std::hex << sal::version::number << std::endl;
  std::cout << std::hex << sal::version::c_str << std::endl;

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
