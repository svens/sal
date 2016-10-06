/// \addtogroup program_options
#pragma once

/**
 * \file sal/program_options/argument_map.hpp
 * Program options' arguments
 */


#include <sal/config.hpp>
#include <sal/error.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>


namespace sal { namespace program_options {
__sal_begin


class option_set_t;


/**
 * Program options' arguments. This class is returned by option_set_t parsing
 * methods.
 */
class argument_map_t
{
public:

  /**
   * Vector of strings
   */
  using string_list_t = std::vector<std::string>;


  /**
   * Return true if \a option has arguments
   */
  bool has (const std::string &option) const noexcept
  {
    return arguments_.find(option) != arguments_.end();
  }


  /**
   * Return list of arguments for \a option or empty vector if none
   */
  const string_list_t &operator[] (const std::string &option) const
  {
    auto it = arguments_.find(option);
    if (it != arguments_.end())
    {
      return *it->second;
    }
    static const string_list_t empty;
    return empty;
  }


  /**
   * Return list of positional arguments
   */
  const string_list_t &positional_arguments () const noexcept
  {
    return positional_arguments_;
  }


private:

  using string_list_ptr = std::shared_ptr<string_list_t>;
  std::map<std::string, string_list_ptr> arguments_{};
  string_list_t positional_arguments_{};

  argument_map_t () = default;

  friend class option_set_t;
};


__sal_end
}} // namespace sal::program_options

/// \}
