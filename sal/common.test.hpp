#pragma once

#define GTEST_HAS_TR1_TUPLE 0
#include <gtest/gtest.h>


namespace sal_test {


class fixture
  : public testing::Test
{
public:

  const std::string case_name = case_name_();


private:

  static std::string case_name_ ()
  {
    if (auto *info = testing::UnitTest::GetInstance()->current_test_info())
    {
      std::string name = info->test_case_name();
      name += '.';
      name += info->name();
      return name;
    }
    return "<unknown>";
  }
};


template <typename T>
class with_type
  : public fixture
{
};


template <typename T>
class with_value
  : public fixture
  , public testing::WithParamInterface<T>
{
};


} // namespace sal_test
