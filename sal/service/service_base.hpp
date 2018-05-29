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
   * Service logger related members.
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

    /**
     * Asynchronous logger worker. It launches separate thread receiving
     * messages and writing those to sink.
     */
    logger::async_worker_t worker;
  } logger;


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


  ///\{
  // Internal: helpers for sal_log macros to disguise this as channel_t

  auto is_enabled () noexcept
  {
    return logger.worker.default_channel().is_enabled();
  }

  auto make_event ()
  {
    return logger.worker.default_channel().make_event();
  }

  ///\}
};


} // service


__sal_end
