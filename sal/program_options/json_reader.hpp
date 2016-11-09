/// \addtogroup program_options
#pragma once

/**
 * \file sal/program_options/json_reader.hpp
 * JSON/SJSON/HJSON config file
 *
 * Different syntaxes are described at:
 *  * JSON http://json.org
 *  * HJSON http://hjson.org/syntax.html
 *  * SJSON http://help.autodesk.com/cloudhelp/ENU/Stingray-Help/stingray_help/managing_content/sjson.html
 *  * TOML https://github.com/toml-lang/toml
 *
 * SAL library implementation follows mostly HJSON syntax with influences from
 * SJSON/TOML and further simplifications:
 *
 *  * root object doesn't have to be surrounded with curly braces
 *
 *  * both equal sign \c = or colon \c : can be used to define key/value pairs
 *
 *  * keys can be either bare or quoted: see TOML
 *    https://github.com/toml-lang/toml#user-content-keyvalue-pair
 *
 *  * there are multiple string types:
 *    - basic, multi-line basic, literal, multi-line literal string: see TOML
 *      https://github.com/toml-lang/toml#user-content-string
 *    - quoteless string: everything until newline but trailing whitespaces
 *      are trimmed
 *
 *  * all keys and values are read as string (i.e. no booleans, integers etc)
 *
 *  * if array elements are on own lines, they don't need to be separated with
 *    commas (but they can be). Also, last value can have trailing comma.
 *
 * Limitations compared to JSON/HJSON/SJSON
 *  * array elements are only simple values, no objects or arrays are allowed
 */


#include <sal/config.hpp>
#include <iosfwd>
#include <memory>
#include <string>


namespace sal { namespace program_options {
__sal_begin


class option_set_t;


/**
 * JSON-format config file parser for option_set_t::load_from().
 *
 * When parsing input, returned values are flattened i.e. returned as flat
 * key/value pairs where value is prefixed with parent object name. Example:
 * \code
 * parent = {
 *   # return option = "parent.key" / argument = "value"
 *   key = value
 *
 *   # return multiple pairs with same option name:
 *   #  option = "parent.array" / argument = "1"
 *   #  option = "parent.array" / argument = "2"
 *   array = [ 1, 2, ]
 * }
 * \endcode
 */
class json_reader_t
{
public:

  /**
   * Construct JSON reader using input \a input.
   */
  json_reader_t (std::istream &input);

  ~json_reader_t ();


  /**
   * Get next option/argument pair.
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
