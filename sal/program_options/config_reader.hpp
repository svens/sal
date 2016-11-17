/// \addtogroup program_options
#pragma once

/**
 * \file sal/program_options/config_reader.hpp
 * Configuration file parser
 *
 * For more information, see \ref sal_program_options_config
 */


#include <sal/config.hpp>
#include <iosfwd>
#include <memory>
#include <string>


namespace sal { namespace program_options {
__sal_begin


class option_set_t;


/**
 * Structured configuration file parser for option_set_t::load_from()
 *
 * To parse file pass input stream to constructor and repeatedly call
 * operator() until it returns false. Each call will assign next read
 * option/argument pair.
 *
 * While parsing, structured configuration is flattened i.e. returned as
 * key/value pairs where key name has path from root object to final key.
 * Example:
 * \code
 * parent = {
 *   // return option = "parent.key" / argument = "value"
 *   key = value
 *
 *   // return multiple pairs with same option name:
 *   //  option = "parent.array" / argument = "1"
 *   //  option = "parent.array" / argument = "2"
 *   array = [ 1, 2, ]
 * }
 * \endcode
 */
class config_reader_t
{
public:

  /**
   * Construct JSON reader using \a input.
   */
  config_reader_t (std::istream &input);

  ~config_reader_t ();


  /**
   * Get next option/argument pair. \a option and \a argument are valid only
   * if method call returned \c true.
   *
   * \see option_set_t::load_from()
   */
  bool operator() (const option_set_t &,
    std::string *option,
    std::string *argument
  );


private:

  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};


__sal_end
}} // namespace sal::program_options

/// \}
