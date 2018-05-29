#pragma once

/**
 * \file sal/service/application.hpp
 * Simple application class.
 */

#include <sal/config.hpp>
#include <sal/program_options/argument_map.hpp>
#include <sal/program_options/option_set.hpp>
#include <iosfwd>
#include <string>


__sal_begin


namespace service {


/**
 * Simple application class. It provides common framework to parse command
 * line arguments and possible configuration file.
 *
 * Typical usage:
 * \code
 * int main (int argc, const char *argv[])
 * {
 *   sal::application_t application(argc, argv, application_options());
 *   if (application.help_requested())
 *   {
 *     return application.help(std::cout);
 *   }
 *
 *   // application logic
 *
 *   return EXIT_SUCCESS;
 * }
 * \endcode
 */
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
   * Gathered list of application options. Option names 'help', 'h', 'config'
   * and 'c' are reserved for this class.
   */
  const program_options::option_set_t options;

  /**
   * Arguments provided from command line.
   */
  const program_options::argument_map_t command_line;

  /**
   * Arguments loaded from config file. Unless command line argument
   * '--config' specifies different config file name, default config is loaded
   * from current working directory from file named \c name + ".conf".
   */
  const program_options::argument_map_t config_file;


  /**
   * Create new application object with command line arguments loaded from
   * \a argv. \a options describe known arguments. This class itself adds
   * following options:
   *  - 'help' or 'h': print help screen with known options
   *  - 'config' or 'c': load configuration from specified file
   *
   * This constructor does not handle help itself. If help_requested() returns
   * true, it is application responsibility to invoke help() and exit.
   */
  application_t (int argc, const char *argv[],
    program_options::option_set_t options
  );


  /**
   * Returns true if "--help" or "-h" is specified on command line.
   */
  bool help_requested () const noexcept
  {
    return command_line.has("help");
  }


  /**
   * Print help to stream \a os and return EXIT_SUCCESS.
   */
  int help (std::ostream &os) const;
};


} // service


__sal_end
