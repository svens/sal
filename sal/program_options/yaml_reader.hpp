/// \addtogroup program_options
#pragma once

/**
 * \file sal/program_options/yaml_reader.hpp
 * YAML-like config file
 */


#include <sal/config.hpp>
#include <iosfwd>
#include <memory>
#include <string>


namespace sal { namespace program_options {
__sal_begin


class option_set_t;


/**
 * YAML-like config file parser for option_set_t::load_from().
 */
class yaml_reader_t
{
public:

  /**
   * Construct YAML reader using input \a input.
   */
  yaml_reader_t (std::istream &input);

  ~yaml_reader_t ();


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
