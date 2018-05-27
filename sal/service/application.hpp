#pragma once

/**
 * \file sal/service/application.hpp
 */

#include <sal/config.hpp>
#include <sal/logger/async_worker.hpp>
#include <sal/program_options/argument_map.hpp>
#include <sal/program_options/option_set.hpp>
#include <ostream>
#include <string>


__sal_begin


namespace service {


class application_t
{
public:

  /**
   * Application name extracted from argv[0] without path (and without
   * extension on Windows platform)
   */
  const std::string name;

  /**
   * Application executable path extracted from argv[0] with final directory
   * separator character.
   */
  const std::string path;

  /**
   * Gathered list of application options. All option names starting with
   * 'service.' are reserved for class application_t. Also, option names
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
     * Directory where application logs are sent. Used only if sink is file.
     */
    const std::string dir;

    /**
     * Application logger channel sink. Possible values:
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


  application_t (int argc, const char *argv[],
    program_options::option_set_t options
  );


  bool help_requested () const noexcept
  {
    return command_line.has("help");
  }


  int print_help (std::ostream &os) const;


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
