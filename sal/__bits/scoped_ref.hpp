#pragma once

#include <sal/config.hpp>
#include <utility>

#if __sal_os_darwin
  #include <CoreFoundation/CFBase.h>
#endif

__sal_begin


namespace __bits {

#if __sal_os_darwin // {{{1

template <typename T>
using native_ref = ::CFTypeRef;

template <typename T>
inline native_ref<T> inc_ref (native_ref<T> ref) noexcept
{
  return ::CFRetain(ref);
}

template <typename T>
inline void dec_ref (native_ref<T> ref) noexcept
{
  ::CFRelease(ref);
}

#elif __sal_os_windows // {{{1

template <typename T>
using native_ref = T;

template <typename T>
inline native_ref<T> inc_ref (native_ref<T> ref) noexcept
{
  return ref;
}

template <typename T>
inline void dec_ref (native_ref<T> ref) noexcept
{
  (void)ref;
}

#endif // }}}1

} // namespace __bits


template <typename T,
  __bits::native_ref<T>(*IncRef)(__bits::native_ref<T>) = __bits::inc_ref<T>,
  void(*DecRef)(__bits::native_ref<T>) = __bits::dec_ref<T>
>
struct scoped_ref
{
  T ref{};

  scoped_ref () = default;

  scoped_ref (T ref) noexcept
    : ref(ref)
  {}

  scoped_ref (const scoped_ref &that) noexcept
    : ref(that.ref)
  {
    if (ref)
    {
      IncRef(ref);
    }
  }

  scoped_ref &operator= (const scoped_ref &that) noexcept
  {
    auto copy(that);
    swap(copy);
    return *this;
  }

  scoped_ref (scoped_ref &&that) noexcept
    : ref(that.ref)
  {
    that.ref = nullptr;
  }

  scoped_ref &operator= (scoped_ref &&that) noexcept
  {
    auto tmp(std::move(that));
    swap(tmp);
    return *this;
  }

  ~scoped_ref () noexcept
  {
    if (ref)
    {
      DecRef(ref);
    }
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
