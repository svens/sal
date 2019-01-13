#pragma once

#include <sal/config.hpp>
#include <memory>
#include <string>

#define GTEST_HAS_TR1_TUPLE 0
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


#if !(_MSC_VER && _DEBUG)
// TODO: bug in MSVC for failing_string

//
// https://howardhinnant.github.io/allocator_boilerplate.html
//

template <typename T>
class failing_allocator
{
public:

  using value_type = T;
  using pointer = value_type *;

  failing_allocator () noexcept = default;

  template <typename U>
  failing_allocator (const failing_allocator<U> &) noexcept
  { }

  pointer allocate (size_t)
  {
    throw std::bad_alloc();
  }

  void deallocate (pointer, size_t) noexcept
  { }
};


using failing_string = std::basic_string<char,
  std::char_traits<char>,
  failing_allocator<char>
>;

#endif


} // namespace sal_test
