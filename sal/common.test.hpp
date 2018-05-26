#pragma once

#include <sal/config.hpp>
#include <gtest/gtest.h>


namespace sal_test {


class fixture
  : public testing::Test
{
public:

  const bool on_appveyor_ci = on_appveyor_ci_();
  const bool on_travis_ci = on_travis_ci_();
  const bool on_ci = on_appveyor_ci || on_travis_ci;
  const std::string case_name = case_name_();


private:

  static bool on_appveyor_ci_ ()
  {
#if __sal_os_windows
    size_t required_size = 0;
    getenv_s(&required_size, nullptr, 0, "APPVEYOR");
    if (required_size != 0)
    {
      return true;
    }
#endif
    return false;
  }

  static bool on_travis_ci_ ()
  {
#if __sal_os_windows
    return false;
#else
    return std::getenv("TRAVIS") != nullptr;
#endif
  }

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
