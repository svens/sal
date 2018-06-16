#include <sal/service/service_base.hpp>
#include <sal/error.hpp>
#include <sal/logger/file_sink.hpp>
#include <sal/logger/logger.hpp>
#include <sal/program_options/command_line.hpp>
#include <sal/program_options/config_reader.hpp>
#include <condition_variable>
#include <exception>
#include <mutex>


__sal_begin


namespace service {


using namespace std::chrono_literals;


namespace {


// service configuration variables read from command line or config file
static const std::string
  service_logger_dir = "service.logger.dir",
  service_logger_sink = "service.logger.sink",
  service_thread_count = "service.thread_count",
  service_control_port = "service.control.port";


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
    .add({service_control_port},
      requires_argument("INT", 2367),
      help("service controller listening port\n"
        "(default: 2367, disable: 0)"
      )
    )
  ;
  return option_set;
}


using service_config_t = std::remove_const_t<decltype(service_base_t::config)>;


inline void throw_invalid_value [[noreturn]] (const std::string &name)
{
  throw_runtime_error(name, ": invalid value");
}


inline size_t to_count (const std::string &name, const std::string &value)
{
  try
  {
    if (auto result = std::stoul(value))
    {
      return result;
    }
  }
  catch (...)
  { }
  throw_invalid_value(name);
}


inline uint16_t to_port (const std::string &name, const std::string &value)
{
  try
  {
    auto result = std::stoul(value);
    if (result <= std::numeric_limits<uint16_t>::max())
    {
      return static_cast<uint16_t>(result);
    }
  }
  catch (...)
  { }
  throw_invalid_value(name);
}


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
    to_count(
      service_thread_count,
      options.back_or_default(service_thread_count,
        {config_file, command_line}
      )
    ),

    // control
    {
      to_port(
        service_control_port,
        options.back_or_default(service_control_port,
          {config_file, command_line}
        )
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


struct service_base_t::impl_t
{
  // Sentinel is randomly chosen thread that:
  // - invokes event_handler.service_start
  // - starts control sessions acceptor
  // - closes control sessions on exit
  //
  // Once control acceptor has started, no thread can exit until all control
  // sessions are stopped because any thread's context might be still in use

  service_base_t &service;
  std::exception_ptr exception{};

  std::mutex mutex{};
  std::vector<std::thread> threads{};
  std::condition_variable condition{};
  size_t ready_count{}, stop_count{};

  impl_t (service_base_t &service) noexcept
    : service(service)
  {}


  ~impl_t () noexcept
  {
    for (auto &thread: threads)
    {
      thread.join();
    }
  }


  std::unique_lock<std::mutex> lock () noexcept
  {
    return std::unique_lock(mutex);
  }


  void store_current_exception () noexcept
  {
    if (auto guard = lock(); !exception)
    {
      exception = std::current_exception();
    }
  }


  void rethrow_current_exception ()
  {
    if (auto guard = lock(); exception)
    {
      std::rethrow_exception(exception);
    }
  }


  void suspend_until_threads_ready (std::unique_lock<std::mutex> &guard)
    noexcept
  {
    while (ready_count != service.config.thread_count)
    {
      condition.wait(guard);
    }
  }


  void suspend_until_threads_stop (std::unique_lock<std::mutex> &guard)
    noexcept
  {
    while (stop_count != service.config.thread_count)
    {
      condition.wait(guard);
    }
  }


  bool start (event_handler_t &event_handler,
    net::async_service_t::context_t &context
  ) noexcept;

  void poll (event_handler_t &event_handler) noexcept;

  void stop (bool sentinel) noexcept;
};


bool service_base_t::impl_t::start (event_handler_t &event_handler,
  net::async_service_t::context_t &context) noexcept
{
  auto guard = lock();

  if (++ready_count < service.config.thread_count)
  {
    suspend_until_threads_ready(guard);
    return false;
  }

  try
  {
    event_handler.service_start(context);
  }
  catch (...)
  {
    exception = std::current_exception();
  }

  condition.notify_all();
  return true;
}


void service_base_t::impl_t::stop (bool sentinel) noexcept
{
  auto guard = lock();

  if (sentinel)
  {
  }

  if (++stop_count < service.config.thread_count)
  {
    suspend_until_threads_stop(guard);
  }
  else
  {
    condition.notify_all();
  }
}


void service_base_t::impl_t::poll (event_handler_t &event_handler) noexcept
{
  auto context = service.async_net.make_context();
  auto sentinel = start(event_handler, context);

  try
  {
    std::error_code error;
    while (service.exit_code_ == no_exit)
    {
      if (auto ev = context.poll(10ms, error))
      {
        (void)ev;
      }
    }
  }
  catch (...)
  {
    store_current_exception();
  }

  stop(sentinel);
}


service_base_t::service_base_t (int argc, const char *argv[],
    program_options::option_set_t option_set)
  : application_t(argc, argv, with_service_options(option_set))
  , config(service_config(options, command_line, config_file))
  , logger(service_logger(command_line, config))
  , async_net()
  , impl_()
{
  if (config.logger.sink == "null")
  {
    logger.default_channel().set_enabled(false);
  }
}


service_base_t::~service_base_t () noexcept
{ }


void service_base_t::start (event_handler_t &event_handler)
{
  impl_ = std::make_unique<impl_t>(*this);
  auto guard = impl_->lock();

  while (impl_->threads.size() < config.thread_count)
  {
    impl_->threads.emplace_back(&service_base_t::impl_t::poll, impl_.get(),
      std::ref(event_handler)
    );
  }

  impl_->suspend_until_threads_ready(guard);

  now_ = sal::now();
}


bool service_base_t::tick (event_handler_t &event_handler,
  const std::chrono::milliseconds &tick_interval)
{
  if (impl_->exception)
  {
    exit(EXIT_FAILURE);
  }

  if (exit_code_ != no_exit)
  {
    return false;
  }

  try
  {
    event_handler.service_tick(now_);
    if (exit_code_ == no_exit)
    {
      std::this_thread::sleep_for(tick_interval);
      return true;
    }
  }
  catch (...)
  {
    impl_->store_current_exception();
    exit(EXIT_FAILURE);
  }

  auto guard = impl_->lock();
  impl_->suspend_until_threads_stop(guard);
  return false;
}


void service_base_t::stop (event_handler_t &event_handler)
{
  event_handler.service_stop();
  impl_->rethrow_current_exception();
}


} // namespace service


__sal_end
