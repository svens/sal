#include <sal/service/application.hpp>
#include <sal/error.hpp>
#include <sal/program_options/command_line.hpp>
#include <sal/program_options/config_reader.hpp>
#include <fstream>
#include <string_view>


__sal_begin


namespace service {


namespace {


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


std::string_view app_name (int argc, const char *argv[])
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


std::string_view app_path (int argc, const char *argv[])
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


program_options::option_set_t app_options (const std::string &app_name,
  sal::program_options::option_set_t option_set)
{
  using namespace program_options;

  auto config_default = app_name + ".conf";
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
  ;
  return option_set;
}


program_options::argument_map_t app_config (
  const program_options::option_set_t &options,
  const program_options::argument_map_t &command_line)
{
  if (command_line.has("help"))
  {
    std::istringstream iss;
    return options.parse<program_options::config_reader_t>(iss);
  }
  std::ifstream file(options.back_or_default("config", {command_line}));
  return options.parse<program_options::config_reader_t>(file);
}


} // namespace


application_t::application_t (int argc, const char *argv[],
    program_options::option_set_t option_set)
  : name(app_name(argc, argv))
  , path(app_path(argc, argv))
  , options(app_options(name, option_set))
  , command_line(options.parse<program_options::command_line_t>(argc, argv))
  , config_file(app_config(options, command_line))
{ }


int application_t::help (std::ostream &os) const
{
  os
    << "usage:\n  " << name << " [options]\n\n"
    << "options:" << options
    << '\n';
  return EXIT_SUCCESS;
}


} // namespace service


__sal_end
