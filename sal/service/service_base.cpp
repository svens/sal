#include <sal/service/service_base.hpp>
#include <sal/error.hpp>
#include <sal/logger/file_sink.hpp>
#include <sal/program_options/command_line.hpp>
#include <sal/program_options/config_reader.hpp>


__sal_begin


namespace service {


namespace {


// service configuration variables read from command line or config file
static const std::string
  service_logger_dir = "service.logger.dir",
  service_logger_sink = "service.logger.sink";


program_options::option_set_t with_service_options (
  sal::program_options::option_set_t option_set)
{
  using namespace program_options;

  option_set
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
        "filename: send service log messages to specified file\n"
        "null: disable service logging\n"
      )
    )
  ;
  return option_set;
}


logger::sink_ptr make_logger_sink (
  const std::string &dir,
  const std::string &sink)
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


std::remove_const_t<decltype(service_base_t::logger)> service_logger (
  const program_options::option_set_t &options,
  const program_options::argument_map_t &command_line,
  const program_options::argument_map_t &config_file)
{
  if (command_line.has("help"))
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
    {logger::set_channel_sink(make_logger_sink(dir_name, sink_name))},
  };
}


} // namespace


service_base_t::service_base_t (int argc, const char *argv[],
    program_options::option_set_t option_set)
  : application_t(argc, argv, with_service_options(option_set))
  , logger(service_logger(options, command_line, config_file))
{
  if (logger.sink == "null")
  {
    logger.worker.default_channel().set_enabled(false);
  }
}


} // namespace service


__sal_end
