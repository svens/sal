#pragma once

/**
 * \file sal/service/service_base.hpp
 * Service base class.
 */

#include <sal/config.hpp>
#include <sal/service/application.hpp>
#include <sal/logger/async_worker.hpp>
#include <sal/net/async_service.hpp>


__sal_begin


namespace service {


/**
 * Basic service base class. It provides functionality common to networking
 * services.
 */
class service_base_t
  : public application_t
{
public:

  /**
   * This class creation time.
   */
  const sal::time_t start_time = sal::now();

  /**
   * Service configuration collected from config file and command line.
   */
  struct
  {
    /**
     * Service logger related configuration.
     */
    struct
    {
      /**
       * Directory where service logs are sent. Used only if sink is file.
       * Configurable using option "service.logger.dir".
       */
      std::string dir;

      /**
       * Service logger channel sink. Possible values:
       *  - stdout: std::cout
       *  - null: std::cout with channel being disabled
       *  - other values are assumed to be name from which actual filename is
       *    composed. This value is passed to logger::file()
       *
       * Configurable using option "service.logger.sink".
       */
      std::string sink;
    } logger;

    /**
     * Number of service worker threads. Defaults to number of cores.
     * Configurable using option "service.thread_count".
     */
    size_t thread_count;

    /**
     * Service controller related configuration
     */
    struct
    {
      /**
       * Port where accepting incoming control sessions.
       */
      uint16_t port;
    } control;
  } const config;


  /**
   * Service logger worker.
   */
  logger::async_worker_t logger;


  /**
   * Asynchronous networking service.
   */
  net::async_service_t async_net;


  /**
   * Create new service object with command line arguments loaded from
   * \a argv. \a options describe known arguments.
   *
   * For future extending, this class reserves adding configuration with
   * "service." prefix.
   *
   * Constructor does not handle help itself. If help_requested() returns
   * true, it is service responsibility to invoke help() and exit.
   */
  service_base_t (int argc, const char *argv[],
    program_options::option_set_t options
  );


  ~service_base_t () noexcept;


  struct event_handler_t
  {
    virtual ~event_handler_t () = default;

    virtual void service_start (net::async_service_t::context_t &context) = 0;
    virtual void service_stop () = 0;
    virtual void service_tick (const sal::time_t &now) = 0;
  };


  template <typename Rep, typename Period>
  int run (event_handler_t &event_handler,
    const std::chrono::duration<Rep, Period> &tick_interval)
  {
    using namespace std::chrono;
    auto tick_interval_ms = duration_cast<milliseconds>(tick_interval);

    start(event_handler);
    while (tick(event_handler, tick_interval_ms))
    {
      now_ = sal::now();
    }
    stop(event_handler);

    return exit_code_;
  }


  int run (event_handler_t &event_handler)
  {
    using namespace std::chrono_literals;
    return run(event_handler, 1s);
  }


  /**
   * Set exit code.
   */
  bool exit (int code) noexcept
  {
    int run_code = no_exit;
    return exit_code_.compare_exchange_strong(run_code, code);
  }


  std::chrono::seconds uptime () const noexcept
  {
    using namespace std::chrono;
    return duration_cast<seconds>(now_ - start_time);
  }


  ///\{
  // Internal: helpers for sal_log macros to disguise this as channel_t

  auto is_logger_channel_enabled () noexcept
  {
    return logger.default_channel().is_logger_channel_enabled();
  }

  auto make_logger_event ()
  {
    return logger.default_channel().make_logger_event();
  }

  ///\}


protected:

  void start (event_handler_t &event_handler);

  bool tick (event_handler_t &event_handler,
    const std::chrono::milliseconds &tick_interval
  );

  void stop (event_handler_t &event_handler);


private:

  static constexpr int no_exit = -1;
  std::atomic<int> exit_code_{no_exit};
  sal::time_t now_ = start_time;

  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};


} // service


__sal_end
