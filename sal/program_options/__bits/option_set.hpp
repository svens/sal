#pragma once

#include <sal/config.hpp>
#include <sstream>
#include <string>


namespace sal { namespace program_options { namespace __bits {


struct option_t
{
  // * unit.empty() -- no argument
  // * !unit.empty() && requires_argument -- requires argument
  // * !unit.empty() && !requires_argument -- optional argument
  std::string unit{}, help{}, default_value{};
  bool requires_argument = false;

  template <typename... Args>
  option_t (Args &&...args)
  {
    bool unused[] = { std::forward<Args>(args).update(*this)..., false};
    (void)unused;
  }
};


using option_ptr = std::shared_ptr<option_t>;


struct requires_argument_t
{
  const std::string &unit;

  explicit requires_argument_t (const std::string &unit)
    : unit(unit)
  {}

  bool update (option_t &option) const
  {
    option.requires_argument = true;
    option.unit = unit;
    return false;
  }
};


struct optional_argument_t
{
  const std::string &unit;

  explicit optional_argument_t (const std::string &unit)
    : unit(unit)
  {}

  bool update (option_t &option) const
  {
    option.requires_argument = false;
    option.unit = unit;
    return false;
  }
};


struct help_t
{
  const std::string &text;

  explicit help_t (const std::string &text)
    : text(text)
  {}

  bool update (option_t &option) const
  {
    option.help = text;
    return false;
  }
};


template <typename T>
struct default_value_t
{
  const T &default_value;

  default_value_t (const T &default_value)
    : default_value(default_value)
  {}

  bool update (option_t &option) const
  {
    std::ostringstream oss;
    oss << default_value;
    option.default_value = oss.str();
    return false;
  }
};


}}} // namespace sal::program_options::__bits
