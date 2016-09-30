#pragma once

#include <sal/common.test.hpp>
#include <sal/program_options/option_set.hpp>
#include <initializer_list>
#include <string>


namespace sal_test {


class hardcoded_config_t
{
public:

  using value_t = std::pair<std::string, std::string>;
  using value_list_t = std::vector<value_t>;


  hardcoded_config_t (std::initializer_list<value_t> data)
    : data_(data)
    , it_(data_.begin())
  {}


  const value_t &operator() (const sal::program_options::option_set_t &)
  {
    if (it_ != data_.end())
    {
      return *it_++;
    }
    static const value_t end = { "", "" };
    return end;
  }


private:

  value_list_t data_;
  value_list_t::const_iterator it_;
};


} // namespace sal_test
