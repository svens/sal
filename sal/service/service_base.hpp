#pragma once

/**
 * \file sal/service/service_base.hpp
 * Service base class.
 */

#include <sal/config.hpp>
#include <sal/service/application.hpp>
#include <sal/logger/async_worker.hpp>


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
      const std::string dir;

      /**
       * Service logger channel sink. Possible values:
       *  - stdout: std::cout
       *  - null: std::cout with channel being disabled
       *  - other values are assumed to be name from which actual filename is
       *    composed. This value is passed to logger::file()
       *
       * Configurable using option "service.logger.sink".
       */
      const std::string sink;
    } const logger;
  } const config;


  /**
   * Service logger worker.
   */
  logger::async_worker_t logger;


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


  struct event_handler_t
  {
    virtual ~event_handler_t () = default;

    virtual void service_start ()
    { }

    virtual void service_stop ()
    { }

    virtual void service_tick (const sal::time_t &now)
    {
      (void)now;
    }
  };


  template <typename Rep, typename Period>
  int run (event_handler_t &event_handler,
    const std::chrono::duration<Rep, Period> &tick_interval)
  {
    using namespace std::chrono;
    const auto tick_interval_ms = duration_cast<milliseconds>(tick_interval);

    start(event_handler);
    for (now_ = sal::now();  exit_code_ < 0;  now_ = sal::now())
    {
      tick(event_handler, tick_interval_ms);
    }
    stop(event_handler);
    return exit_code_;
  }


  int run (event_handler_t &event_handler)
  {
    constexpr auto tick_interval = std::chrono::seconds{1};
    return run(event_handler, tick_interval);
  }


  /**
   * Set exit code. Allow success turn into error but not other way around.
   */
  void exit (int code) noexcept
  {
    if (exit_code_ < 1)
    {
      exit_code_ = code;
    }
  }


  std::chrono::seconds uptime () const noexcept
  {
    using namespace std::chrono;
    return duration_cast<seconds>(now_ - start_time_);
  }


  ///\{
  // Internal: helpers for sal_log macros to disguise this as channel_t

  auto is_enabled () noexcept
  {
    return logger.default_channel().is_enabled();
  }

  auto make_event ()
  {
    return logger.default_channel().make_event();
  }

  ///\}


private:

  int exit_code_ = -1;
  sal::time_t now_ = sal::now(), start_time_ = now_;

  void start (event_handler_t &event_handler);
  void tick (event_handler_t &event_handler,
    const std::chrono::milliseconds &tick_interval
  );
  void stop (event_handler_t &event_handler);
};


} // service


__sal_end