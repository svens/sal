#include <sal/service/service_base.hpp>
#include <sal/error.hpp>
#include <sal/logger/file_sink.hpp>
#include <sal/program_options/command_line.hpp>
#include <sal/program_options/config_reader.hpp>
#include <condition_variable>
#include <exception>
#include <mutex>


__sal_begin


namespace service {


namespace {


// service configuration variables read from command line or config file
static const std::string
  service_logger_dir = "service.logger.dir",
  service_logger_sink = "service.logger.sink",
  service_thread_count = "service.thread_count";


program_options::option_set_t with_service_options (
  sal::program_options::option_set_t option_set)
{
  using namespace program_options;

  auto service_thread_count_default = std::to_string(
    std::thread::hardware_concurrency()
  );

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
    .add({service_thread_count},
      requires_argument("INT", service_thread_count_default),
      help("number of worker threads to launch.\n"
        "(default: " + service_thread_count_default + ')'
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

  // config
  return
  {
    // logger
    {
      // dir
      options.back_or_default(service_logger_dir,
        {config_file, command_line}
      ),

      // sink
      options.back_or_default(service_logger_sink,
        {config_file, command_line}
      ),
    },

    // thread_count
    std::stoul(
      options.back_or_default(service_thread_count,
        {config_file, command_line}
      )
    ),
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


struct service_base_t::impl_t
{
  std::mutex mutex{};
  std::condition_variable all_ready{};
  std::vector<std::thread> threads{};
  size_t ready_count{};
  std::exception_ptr exception{};


  ~impl_t () noexcept
  {
    for (auto &thread: threads)
    {
      thread.join();
    }
  }


  void service_start (service_base_t &service,
    event_handler_t &event_handler,
    net::async_service_t::context_t &context) noexcept
  {
    std::lock_guard lock(mutex);
    if (++ready_count == service.config.thread_count)
    {
      try
      {
        event_handler.service_start(context);
      }
      catch (...)
      {
        exception = std::current_exception();
      }
      all_ready.notify_one();
    }
  }


  void store_current_exception () noexcept
  {
    std::lock_guard lock(mutex);
    if (!exception)
    {
      exception = std::current_exception();
    }
  }


  void rethrow_current_exception ()
  {
    std::lock_guard lock(mutex);
    if (exception)
    {
      std::rethrow_exception(exception);
    }
  }
};


service_base_t::service_base_t (int argc, const char *argv[],
    program_options::option_set_t option_set)
  : application_t(argc, argv, with_service_options(option_set))
  , config(service_config(options, command_line, config_file))
  , logger(service_logger(command_line, config))
  , async_net()
  , impl_(std::make_unique<impl_t>())
{
  if (config.logger.sink == "null")
  {
    logger.default_channel().set_enabled(false);
  }
}


service_base_t::~service_base_t () noexcept
{ }


void service_base_t::service_start (event_handler_t &event_handler)
{
  std::unique_lock lock(impl_->mutex);

  while (impl_->threads.size() < config.thread_count)
  {
    impl_->threads.emplace_back(&service_base_t::service_poll, this,
      std::ref(event_handler)
    );
  }

  while (impl_->ready_count != config.thread_count)
  {
    impl_->all_ready.wait(lock);
  }
}


void service_base_t::service_poll (event_handler_t &event_handler) noexcept
{
  try
  {
    auto context = async_net.make_context();
    impl_->service_start(*this, event_handler, context);

    std::error_code error;
    while (exit_code_ == -1)
    {
      using namespace std::chrono_literals;
      if (auto ev = context.poll(10ms, error))
      {
      }
    }
  }
  catch (...)
  {
    impl_->store_current_exception();
  }
}


bool service_base_t::service_tick (event_handler_t &event_handler,
  const std::chrono::milliseconds &tick_interval)
{
  try
  {
    impl_->rethrow_current_exception();

    now_ = sal::now();
    event_handler.service_tick(now_);

    if (exit_code_ == -1)
    {
      std::this_thread::sleep_for(tick_interval);
      return true;
    }

    return false;
  }
  catch (...)
  {
    exit(EXIT_FAILURE);
    throw;
  }
}


void service_base_t::service_stop (event_handler_t &event_handler)
{
  event_handler.service_stop();
}


} // namespace service


__sal_end
