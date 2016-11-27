#pragma once

#include <sal/config.hpp>
#include <utility>


__sal_begin


namespace __bits {


// helper class to assign value to \a Class \a Member of type \a T
template <typename T, typename Class, T Class::*Member>
struct member_assign_t
{
  const T value;

  // use \a value
  explicit member_assign_t (const T &value)
    : value(value)
  {}

  // assign \a value_ to \a object \a Member
  void operator() (Class *object) const
  {
    object->*Member = value;
  }
};


// assign \a args to \a object members
// each item from \a arg should be instance of member_assign_t
template <typename Class, typename... Args>
inline void assign_members (Class *object, Args &&...args)
{
  bool unused[] = { (std::forward<Args>(args)(object), false)... };
  (void)unused;
}


// dummy assign \a args to \a object members ie assign no members
template <typename Class>
inline void assign_members (Class *)
{
}


} // namespace __bits


__sal_end
