#pragma once

/**
 * \file sal/service/service_base.hpp
 */

#include <sal/config.hpp>
#include <sal/logger/async_worker.hpp>
#include <sal/logger/logger.hpp>
#include <sal/program_options/argument_map.hpp>
#include <sal/program_options/option_set.hpp>
#include <string>


__sal_begin


namespace service {


class service_base_t
{
public:

  /**
   * Service name extracted from argv[0] without path (and without extension
   * on Windows platform)
   */
  const std::string name;

  /**
   * Service executable path extracted from argv[0] with final directory
   * separator character.
   */
  const std::string path;

  /**
   * Gathered list of application options. All option names starting with
   * 'service.' are reserved for class service_base_t. Also, option names
   * 'help', 'h', 'config' and 'c' are reserved.
   */
  const program_options::option_set_t options;

  /**
   * Arguments provided from command line.
   */
  const program_options::argument_map_t command_line;

  /**
   * Arguments loaded from config file.
   */
  const program_options::argument_map_t config_file;


  struct
  {
    /**
     * Directory where service logs are sent. Used only if sink is file.
     */
    const std::string dir;

    /**
     * Service logger channel sink. Possible values:
     *  - stdout: std::cout
     *  - null: std::cout with channel being disabled
     *  - other values are assumed to be name from which actual filename is
     *    composed. This value is passed to logger::file()
     */
    const std::string sink;

    /**
     * Asynchronous logger worker. It launches separate thread receiving
     * messages and writing those to sink.
     */
    logger::async_worker_t worker;
  } logger;


  /**
   * Number of worker threads to start.
   */
  const size_t worker_count;


  service_base_t (int argc, const char *argv[],
    program_options::option_set_t options
  );


  int run ()
  {
    if (command_line.has("help"))
    {
      return usage();
    }
    return work();
  }


private:

  int usage ();
  int work ();
};


#define sal_svc_log(svc) \
  sal_log((svc).logger.worker.default_channel())

#define sal_svc_log_if(svc,expr) \
  sal_log_if((svc).logger.worker.default_channel(), (expr))


} // service


__sal_end
