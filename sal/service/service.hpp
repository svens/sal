#pragma once

/**
 * \file sal/service/service.hpp
 * Service class.
 */

#include <sal/config.hpp>
#include <sal/service/service_base.hpp>


__sal_begin


namespace service {


template <typename Application>
class service_t
  : public service_base_t
{
public:

  service_t (int argc, const char *argv[],
      program_options::option_set_t options)
    : service_base_t(argc, argv, options)
  { }


  template <typename Rep, typename Period>
  int run (const std::chrono::duration<Rep, Period> &tick_interval)
  {
    return service_base_t::run(event_handler_, tick_interval);
  }


  int run ()
  {
    return service_base_t::run(event_handler_);
  }


private:

  struct event_handler_t final
    : public service_base_t::event_handler_t
  {
    service_base_t &service;

    event_handler_t (service_base_t &service)
      : service(service)
    { }

    void service_start (net::async_service_t::context_t &context) final override
    {
      (void)context;
    }

    void service_tick (const sal::time_t &now) final override
    {
      (void)now;
      service.exit(EXIT_SUCCESS);
    }

    void service_stop () final override
    { }
  } event_handler_{*this};
};


} // service


__sal_end
