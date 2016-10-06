/// \addtogroup program_options
#pragma once

/**
 * \file sal/program_options/option_set.hpp
 * Program options' set description
 */


#include <sal/config.hpp>
#include <sal/__bits/member_assign.hpp>
#include <sal/program_options/argument_map.hpp>
#include <sal/program_options/error.hpp>
#include <initializer_list>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <tuple>


namespace sal { namespace program_options {
__sal_begin


/**
 * Single option settings.
 */
struct option_t
{
  /**
   * Argument requirement setting.
   */
  enum class argument_t
  {
    none,
    optional,
    required
  };

  /**
   * Option argument settings.
   */
  using argument_info_t = std::tuple<
    argument_t,   /**< Argument rquirement settings */
    std::string,  /**< Argument unit. Valid only if argument is optional or
                    required */
    std::string   /**< Default value for argument. Valid only if argument is
                    optional or required */
  >;


  /**
   * Option argument settings.
   */
  argument_info_t argument{argument_t::none, "", ""};


  /**
   * Help text for option.
   */
  std::string help{};


  /**
   * Returns true if option must not have argument.
   */
  bool no_argument () const noexcept
  {
    return std::get<0>(argument) == argument_t::none;
  }


  /**
   * Returns true if option must have argument.
   */
  bool requires_argument () const noexcept
  {
    return std::get<0>(argument) == argument_t::required;
  }


  /**
   * Returns true if option may have argument.
   */
  bool optional_argument () const noexcept
  {
    return std::get<0>(argument) == argument_t::optional;
  }


  /**
   * Return option argument unit name (eg string, int etc). Valid only if
   * argument is optional or required.
   */
  const std::string &unit () const noexcept
  {
    return std::get<1>(argument);
  }


  /**
   * Return textual default value for option argument. Valid only if argument
   * is optional or required.
   */
  const std::string &default_value () const noexcept
  {
    return std::get<2>(argument);
  }


  /// Internal helper
  using assign_argument_t = __bits::member_assign_t<
    argument_info_t, option_t, &option_t::argument
  >;

  /// Internal helper
  using assign_help_t = __bits::member_assign_t<
    std::string, option_t, &option_t::help
  >;
};


/** Shared pointer to option */
using option_ptr = std::shared_ptr<option_t>;


/**
 * Return opaque option setting to indicate option's argument is required.
 * Also set's argument \a unit. Default value is set to empty string.
 */
inline auto requires_argument (const std::string &unit)
{
  return option_t::assign_argument_t(
    { option_t::argument_t::required, unit, "" }
  );
}


/**
 * Return opaque option setting to indicate option's argument is required.
 * Also set's argument \a unit and \a default_value.
 */
template <typename T>
inline auto requires_argument (const std::string &unit, const T &default_value)
{
  std::ostringstream oss;
  oss << default_value;
  return option_t::assign_argument_t(
    { option_t::argument_t::required, unit, oss.str() }
  );
}


/**
 * Return opaque option setting to indicate option's argument is optional.
 * Also set's argument \a unit. Default value is set to empty string.
 */
inline auto optional_argument (const std::string &unit)
{
  return option_t::assign_argument_t(
    { option_t::argument_t::optional, unit, "" }
  );
}


/**
 * Return opaque option setting to indicate option's argument is optional.
 * Also set's argument \a unit and \a default_value.
 */
template <typename T>
inline auto optional_argument (const std::string &unit, const T &default_value)
{
  std::ostringstream oss;
  oss << default_value;
  return option_t::assign_argument_t(
    { option_t::argument_t::optional, unit, oss.str() }
  );
}


/**
 * Return opaque option setting for help text.
 */
inline auto help (const std::string &text)
{
  return option_t::assign_help_t(text);
}


/**
 * Program options' description. It contains list of options' names with
 * corresponding help text and methods to access options' arguments from
 * multiple sources.
 */
class option_set_t
{
public:

  /**
   * Add new option with \a names to set. New option can be configured with
   * \a args.
   *
   * Possible configuration settings:
   *   - requires_argument()
   *   - optional_argument()
   *   - help()
   *
   * This method returns \e this allowing multiple option adding chained
   * together for convenience.
   */
  template <typename... Args>
  option_set_t &add (std::initializer_list<std::string> names, Args &&...args);


  /**
   * Return pointer to \a option settings or nullptr if not found.
   */
  option_ptr find (const std::string &option) const noexcept
  {
    auto it = options_.find(option);
    return it != options_.end() ? it->second : nullptr;
  }


  /**
   * Create option/argument map from \a parser. Parser must provide call
   * operator with API:
   * \code
   * bool operator() (const option_set_t &,
   *   std::string *option, std::string *argument
   * );
   * \endcode
   * It is called until it returns false, indicating end of input data. On
   * each call, parser should set value for \a option and \a argument:
   *   - both option & argument are non-empty: insert into argument_map_t
   *   - option is non-empty & argument is empty: insert option with default
   *     value (or empty if default value was not set for option)
   *   - option is empty & argument is not empty: insert argument into
   *     argument_map_t::positional_arguments() list
   */
  template <typename Parser>
  argument_map_t load_from (Parser &parser) const;


  /**
   * Instantiate object of type \a Parser with \a parser_args and invoke
   * load_from()
   */
  template <typename Parser, typename... ParserArgs>
  argument_map_t parse (ParserArgs &&...parser_args) const
  {
    Parser parser(std::forward<ParserArgs>(parser_args)...);
    return load_from(parser);
  }


  /**
   * Compose and print help for options_set_t (with maximum \a width). If
   * width is not specified (or is 0), current window width is used.
   */
  std::ostream &print (std::ostream &os, size_t width=0) const;


  /**
   * Helper type for specifying list of const argument_map_t references to
   * option/argument extraction methods.
   */
  using argument_map_list_t = std::initializer_list<
    std::reference_wrapper<const argument_map_t>
  >;


  /**
   * Return true if at least one of \a arguments_list contains \a option
   */
  bool has (const std::string &option, argument_map_list_t arguments_list)
    const noexcept
  {
    return front(option, arguments_list) != nullptr;
  }


  /**
   * Return const pointer to string containing first value for \a option in
   * \a arguments_list. If none of \a arguments_list contain \a option,
   * nullptr is returned.
   */
  const std::string *front (const std::string &option,
    argument_map_list_t arguments_list
  ) const noexcept;


  /**
   * Return *front() or default value for \a option if none found.
   */
  const std::string &front_or_default (const std::string &option,
    argument_map_list_t arguments_list) const noexcept
  {
    if (auto *value = front(option, arguments_list))
    {
      return *value;
    }
    return default_value(find(option));
  }


  /**
   * Return const pointer to string containing last value for \a option in
   * \a arguments_list. If none of \a arguments_list contain \a option,
   * nullptr is returned.
   */
  const std::string *back (const std::string &option,
    argument_map_list_t arguments_list
  ) const noexcept;


  /**
   * Return *back() or default value for \a option if none found.
   */
  const std::string &back_or_default (const std::string &option,
    argument_map_list_t arguments_list) const noexcept
  {
    if (auto *value = back(option, arguments_list))
    {
      return *value;
    }
    return default_value(find(option));
  }


  /**
   * Return list of arguments for \a option merged from all \a argument_list.
   * If \a option was not returned by parser, empty list is returned.
   */
  argument_map_t::string_list_t merge (const std::string &option,
    argument_map_list_t arguments_list
  ) const;


  /**
   * Merge and return all postitional arguments from \a arguments_list
   */
  argument_map_t::string_list_t positional_arguments (
    argument_map_list_t arguments_list
  ) const;


private:

  std::map<std::string, option_ptr> options_{};
  std::multimap<option_ptr, std::string> reverse_index_{};

  bool is_valid_option_name (const std::string &name) const noexcept;

  argument_map_t::string_list_t &find_or_add_argument_list (
    const std::string &option,
    const option_ptr &option_p,
    argument_map_t &result
  ) const;

  std::string get_or_make_argument (
    const std::string &option,
    const option_ptr &option_p,
    const std::string &argument
  ) const;

  const std::string &default_value (const option_ptr &option) const noexcept
  {
    static const std::string empty;
    return option ? option->default_value() : empty;
  }
};


template <typename... Args>
option_set_t &option_set_t::add (std::initializer_list<std::string> names,
  Args &&...args)
{
  if (names.size() < 1)
  {
    throw_error<no_option_name>("no option name");
  }

  auto option_p = std::make_shared<option_t>();
  __bits::assign_members(option_p.get(), std::forward<Args>(args)...);

  for (const auto &name: names)
  {
    if (name.empty())
    {
      throw_error<empty_option_name>("empty option name");
    }
    if (!is_valid_option_name(name))
    {
      throw_error<invalid_option_name>("invalid option name: ", name);
    }
    if (!options_.emplace(name, option_p).second)
    {
      throw_error<duplicate_option_name>("duplicate option name: ", name);
    }
    reverse_index_.emplace(option_p, name);
  }

  return *this;
}


template <typename Parser>
argument_map_t option_set_t::load_from (Parser &parser) const
{
  argument_map_t result;

  std::string option, argument;
  while (parser(*this, &option, &argument))
  {
    if (!option.empty())
    {
      auto option_p = find(option);
      auto &arguments = find_or_add_argument_list(option, option_p, result);
      arguments.emplace_back(get_or_make_argument(option, option_p, argument));
    }
    else if (!argument.empty())
    {
      result.positional_arguments_.emplace_back(argument);
    }

    option.clear(), argument.clear();
  }

  return result;
}


/**
 * Print \a option_set help to \a os. Help text is formatted using \a os.width()
 * \see option_set_t::print()
 */
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
