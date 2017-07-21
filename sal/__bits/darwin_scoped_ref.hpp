#pragma once

#if !defined(__APPLE__)
  #error __APPLE__ is undefined
#endif

#include <sal/config.hpp>
#include <CoreFoundation/CFBase.h>
#include <utility>


__sal_begin


template <typename T>
struct scoped_ref
{
  T ref{};

  scoped_ref (T ref = nullptr) noexcept
    : ref(ref)
  {}

  ~scoped_ref () noexcept
  {
    if (ref)
    {
      ::CFRelease(ref);
    }
  }

  scoped_ref (scoped_ref &&that) noexcept
    : ref(that.ref)
  {
    that.ref = nullptr;
  }

  scoped_ref &operator= (scoped_ref &&that) noexcept
  {
    auto tmp(std::move(that));
    swap(that);
    return *this;
  }

  void reset (T that = nullptr) noexcept
  {
    auto tmp(std::move(*this));
    ref = that;
  }

  T release () noexcept
  {
    auto result = ref;
    ref = nullptr;
    return result;
  }

  scoped_ref (const scoped_ref &that) noexcept
    : ref(that.ref)
  {
    if (ref)
    {
      ::CFRetain(ref);
    }
  }

  scoped_ref &operator= (const scoped_ref &that) noexcept
  {
    auto copy(that);
    swap(copy);
    return *this;
  }

  void swap (scoped_ref &that) noexcept
  {
    using std::swap;
    swap(ref, that.ref);
  }

  bool is_null () const noexcept
  {
    return ref == nullptr;
  }

  explicit operator bool () const noexcept
  {
    return ref != nullptr;
  }
};


__sal_end
