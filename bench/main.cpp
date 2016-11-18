#include <bench/bench.hpp>
#include <sal/config.hpp>
#include <sal/program_options/command_line.hpp>
#include <iomanip>
#include <iostream>


namespace {

// initialised in main()
static std::string argv0 = "";

} // namespace


int main (int argc, const char *argv[])
{
  using namespace sal::program_options;

  try
  {
    argv0 = argv[0];

    auto options = bench::options();
    options
      .add({"h", "help"},
        help("display this help and exit")
      )
      .add({"v", "version"},
        help("output version information and exit")
      )
    ;
    auto config = options.parse<command_line_t>(argc, argv);

    if (config.has("help"))
    {
      std::cout << "usage:\n  "
        << argv[0] << " [options]\n\n"
        << "options:"
        << options
        << std::endl
      ;
      return EXIT_SUCCESS;
    }
    else if (config.has("version"))
    {
      std::cout << "sal " << sal::version::c_str << std::endl;
      return EXIT_SUCCESS;
    }

    return bench::run(options, config);
  }
  catch (const std::exception &e)
  {
    std::cout << "failed: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}


namespace bench {


time_point start ()
{
  return clock_type::now();
}


milliseconds stop (time_point start_time, size_t count)
{
  using namespace std::chrono;

  auto msec = duration_cast<milliseconds>(clock_type::now() - start_time);
  if (!msec.count())
  {
    msec = 1ms;
  }

  std::cout << msec.count() << " msec"
    << ", " << count/msec.count() << " count/msec"
    << std::endl;

  return msec;
}


bool in_progress (size_t current, size_t count, size_t &percent)
{
  auto new_percent = current * 100 / count;
  if (current == 1 || percent != new_percent)
  {
    percent = new_percent;
    std::cout << "\r[" << std::setw(3) << percent << "%] " << std::flush;
  }

  return current <= count;
}


int usage (const std::string message)
{
  std::cout << message << '\n'
    << "run: '" << argv0 << " --help' for more information"
    << std::endl;
  return EXIT_FAILURE;
}


} // namespace bench
