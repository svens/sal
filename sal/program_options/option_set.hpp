/// \addtogroup program_options
#pragma once

/**
 * \file sal/program_options/option_set.hpp
 * Program options' set description
 */


#include <sal/config.hpp>
#include <sal/error.hpp>
#include <sal/program_options/__bits/option_set.hpp>
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

  template <typename... Args>
  option_set_t &add (std::initializer_list<std::string> names, Args &&...args);

  std::ostream &print (std::ostream &os, size_t width=0) const;


private:

  std::map<std::string, __bits::option_ptr> options_{};

  bool valid_name (const std::string &name) const noexcept;
};



template <typename... Args>
option_set_t &option_set_t::add (std::initializer_list<std::string> names,
  Args &&...args)
{
  auto option = std::make_shared<__bits::option_t>(
    std::forward<Args>(args)...
  );

  for (const auto &name: names)
  {
    if (name.empty())
    {
      throw_logic_error("empty option name");
    }
    if (!valid_name(name))
    {
      throw_logic_error("invalid option name '", name, '\'');
    }
    if (!options_.emplace(name, option).second)
    {
      throw_logic_error("duplicate option '", name, '\'');
    }
  }

  if (option.unique())
  {
    throw_logic_error("no option name");
  }

  return *this;
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
