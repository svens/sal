/// \addtogroup program_options
#pragma once

/**
 * \file sal/program_options/option_set.hpp
 * Program options' set description
 */


#include <sal/config.hpp>
#include <sal/error.hpp>
#include <sal/program_options/__bits/option_set.hpp>
#include <sal/program_options/argument_map.hpp>
#include <initializer_list>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>


namespace sal { namespace program_options {
__sal_begin


inline auto requires_argument (const std::string &unit)
{
  return __bits::requires_argument_t{unit};
}


inline auto optional_argument (const std::string &unit)
{
  return __bits::optional_argument_t{unit};
}


inline auto help (const std::string &text)
{
  return __bits::help_t{text};
}


template <typename T>
inline auto default_value (const T &value)
{
  return __bits::default_value_t<T>{value};
}


class option_set_t
{
public:

  /**
   */
  template <typename... Args>
  option_set_t &add (std::initializer_list<std::string> names, Args &&...args);


  /**
   */
  template <typename Parser>
  argument_map_t load_from (Parser &parser) const;


  /**
   */
  template <typename Parser, typename... ParserArgs>
  argument_map_t parse (ParserArgs &&...parser_args) const
  {
    Parser parser(std::forward<ParserArgs>(parser_args)...);
    return load_from(parser);
  }


  /**
   */
  std::ostream &print (std::ostream &os, size_t width=0) const;


private:

  std::map<std::string, __bits::option_ptr> options_{};
  std::multimap<__bits::option_ptr, std::string> reverse_index_;

  bool is_valid_option_name (const std::string &name) const noexcept;


  __bits::option_ptr find_option (const std::string &option) const noexcept
  {
    auto it = options_.find(option);
    return it != options_.end() ? it->second : nullptr;
  }


  argument_map_t::string_list_t &find_or_add_argument_list (
    const std::string &option,
    const __bits::option_ptr &option_p,
    argument_map_t &result
  ) const;


  std::string get_or_make_argument (
    const std::string &option,
    const __bits::option_ptr &option_p,
    const std::string &argument
  ) const;
};



template <typename... Args>
option_set_t &option_set_t::add (std::initializer_list<std::string> names,
  Args &&...args)
{
  if (names.size() < 1)
  {
    throw_logic_error("no option name");
  }

  auto option = std::make_shared<__bits::option_t>(
    std::forward<Args>(args)...
  );

  for (const auto &name: names)
  {
    if (name.empty())
    {
      throw_logic_error("empty option name");
    }
    if (!is_valid_option_name(name))
    {
      throw_logic_error("invalid option name '", name, '\'');
    }
    if (!options_.emplace(name, option).second)
    {
      throw_logic_error("duplicate option '", name, '\'');
    }
    reverse_index_.emplace(option, name);
  }

  return *this;
}


template <typename Parser>
argument_map_t option_set_t::load_from (Parser &parser) const
{
  argument_map_t result;

  std::string option, argument;
  for (;;)
  {
    std::tie(option, argument) = parser(*this);
    if (option.empty() && argument.empty())
    {
      break;
    }
    else if (option.empty())
    {
      result.positional_arguments_.emplace_back(argument);
    }
    else
    {
      auto option_p = find_option(option);
      auto &arguments = find_or_add_argument_list(option, option_p, result);
      arguments.emplace_back(get_or_make_argument(option, option_p, argument));
    }
  }

  return result;
}


inline std::ostream &operator<< (std::ostream &os,
  const option_set_t &option_set)
{
  auto width = os.width();
  os.width(0);
  return option_set.print(os, width);
}


__sal_end
}} // namespace sal::program_options

/// \}
