#include <sal/service/application.hpp>
#include <sal/error.hpp>
#include <sal/program_options/command_line.hpp>
#include <sal/program_options/config_reader.hpp>
#include <sal/logger/file_sink.hpp>
#include <fstream>
#include <iostream>
#include <string_view>


__sal_begin


namespace service {


namespace {


// service configuration variables read from command line or config file
static const std::string
  service_logger_dir = "service.logger.dir",
  service_logger_sink = "service.logger.sink";


#if __sal_os_linux || __sal_os_macos

  constexpr std::string_view dir_sep = "/";
  constexpr std::string_view cur_dir = "./";

#elif __sal_os_windows

  // bad msvc
  const std::string_view dir_sep = "/\\";
  const std::string_view cur_dir = ".\\";

  bool ends_with (const std::string_view s, const std::string_view x)
  {
    return s.size() >= x.size()
      && s.compare(s.size() - x.size(), s.npos, x) == 0;
  }

#endif


std::string_view service_name (int argc, const char *argv[])
{
  sal_throw_if(argc == 0);
  std::string_view result = argv[0];

  auto basename_start = result.find_last_of(dir_sep);
  if (basename_start != result.npos)
  {
    result.remove_prefix(basename_start + 1);
  }

#if __sal_os_windows
  if (ends_with(result, ".exe") || ends_with(result, ".com"))
  {
    result.remove_suffix(4);
  }
#endif

  return result;
}


std::string_view service_path (int argc, const char *argv[])
{
  sal_throw_if(argc == 0);
  std::string_view result = argv[0];

  auto basename_start = result.find_last_of(dir_sep);
  if (basename_start != result.npos)
  {
    return result.substr(0, basename_start + 1);
  }

  return cur_dir;
}


program_options::option_set_t service_options (
  const std::string &service_name,
  sal::program_options::option_set_t option_set)
{
  using namespace program_options;

  auto config_default = service_name + ".conf";
  auto worker_count_default = std::to_string(std::thread::hardware_concurrency());

  option_set
    .add({"help", "h"},
      help("display this help and exit")
    )
    .add({"config", "c"},
      requires_argument("STRING", config_default),
      help("config file to load\n"
        "(default: " + config_default + ')'
      )
    )
    .add({service_logger_dir},
      requires_argument("STRING", "logs"),
      help("service logs directory. "
        "This directory is created if it does not exist.\n"
        "(default: logs)"
      )
    )
    .add({service_logger_sink},
      requires_argument("STRING", "stdout"),
      help("service logger destination\n"
        "stdout: send service log messages to stdout (default)\n"
        "filename: send service log messages to specified file"
        "null: disable service logging\n"
      )
    )
  ;
  return option_set;
}


inline bool help (const program_options::argument_map_t &command_line) noexcept
{
  return command_line.has("help");
}


program_options::argument_map_t service_config (
  const program_options::option_set_t &options,
  const program_options::argument_map_t &command_line)
{
  if (help(command_line))
  {
    std::istringstream iss;
    return options.parse<program_options::config_reader_t>(iss);
  }
  std::ifstream file(options.back_or_default("config", {command_line}));
  return options.parse<program_options::config_reader_t>(file);
}


logger::sink_ptr logger_sink (const std::string &dir, const std::string &sink)
{
  if (sink == "stdout" || sink == "null")
  {
    return logger::ostream_sink(std::cout);
  }
  return logger::file(sink,
    logger::set_file_dir(dir),
    logger::set_file_utc_time(true),
    logger::set_file_buffer_size_kb(64)
  );
}


using service_logger_t = std::remove_const_t<decltype(application_t::logger)>;


service_logger_t service_logger (
  const program_options::option_set_t &options,
  const program_options::argument_map_t &command_line,
  const program_options::argument_map_t &config_file)
{
  if (help(command_line))
  {
    return {};
  }

  auto dir_name = options.back_or_default(service_logger_dir,
    {config_file, command_line}
  );
  auto sink_name = options.back_or_default(service_logger_sink,
    {config_file, command_line}
  );

  return
  {
    dir_name,
    sink_name,
    {logger::set_channel_sink(logger_sink(dir_name, sink_name))},
  };
}


} // namespace


application_t::application_t (int argc, const char *argv[],
    program_options::option_set_t option_set)
  : name(service_name(argc, argv))
  , path(service_path(argc, argv))
  , options(service_options(name, option_set))
  , command_line(options.parse<program_options::command_line_t>(argc, argv))
  , config_file(service_config(options, command_line))
  , logger(service_logger(options, command_line, config_file))
{
  if (logger.sink == "null")
  {
    logger.worker.default_channel().set_enabled(false);
  }
}


int application_t::print_help (std::ostream &os) const
{
  os
    << "usage:\n  " << name << " [options]\n\n"
    << "options:" << options
    << '\n';
  return EXIT_SUCCESS;
}


} // namespace service


__sal_end
