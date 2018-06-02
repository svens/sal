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


using service_config_t = std::remove_const_t<decltype(service_base_t::config)>;


service_config_t service_config (
  const program_options::option_set_t &options,
  const program_options::argument_map_t &command_line,
  const program_options::argument_map_t &config_file)
{
  if (command_line.has("help"))
  {
    return {};
  }

  return
  {
    {
      options.back_or_default(service_logger_dir,
        {config_file, command_line}
      ),
      options.back_or_default(service_logger_sink,
        {config_file, command_line}
      ),
    },
  };
}


logger::async_worker_t service_logger (
  const program_options::argument_map_t &command_line,
  const service_config_t &config)
{
  if (command_line.has("help"))
  {
    return {};
  }

  if (config.logger.sink == "stdout" || config.logger.sink == "null")
  {
    return {logger::set_channel_sink(logger::ostream_sink(std::cout))};
  }

  return
  {
    logger::set_channel_sink(
      logger::file(config.logger.sink,
        logger::set_file_dir(config.logger.dir),
        logger::set_file_utc_time(true),
        logger::set_file_buffer_size_kb(64)
      )
    )
  };
}


} // namespace


service_base_t::service_base_t (int argc, const char *argv[],
    program_options::option_set_t option_set)
  : application_t(argc, argv, with_service_options(option_set))
  , config(service_config(options, command_line, config_file))
  , logger(service_logger(command_line, config))
{
  if (config.logger.sink == "null")
  {
    logger.default_channel().set_enabled(false);
  }
}


void service_base_t::start (event_handler_t &event_handler)
{
  event_handler.service_start();
}


void service_base_t::tick (event_handler_t &event_handler,
  const std::chrono::milliseconds &tick_interval)
{
  event_handler.service_tick(now_);
  if (exit_code_ < 0)
  {
    std::this_thread::sleep_for(tick_interval);
  }
}


void service_base_t::stop (event_handler_t &event_handler)
{
  event_handler.service_stop();
}


} // namespace service


__sal_end
