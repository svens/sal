#pragma once

#include <sal/config.hpp>
#include <utility>

#if __sal_os_macos
  #include <CoreFoundation/CFBase.h>
#endif

__sal_begin


namespace __bits {

#if __sal_os_macos //{{{1

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

#elif __sal_os_linux || __sal_os_windows //{{{1

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

#else //{{{1

  #error Unsupported platform

#endif //}}}1

} // namespace __bits


template <typename T,
  __bits::native_ref<T>(*IncRef)(__bits::native_ref<T>) = __bits::inc_ref<T>,
  void(*DecRef)(__bits::native_ref<T>) = __bits::dec_ref<T>
>
struct shared_ref
{
  static constexpr T null{};
  T ref = null;

  shared_ref () = default;

  shared_ref (T ref) noexcept
    : ref(ref)
  {}

  shared_ref (const shared_ref &that) noexcept
    : ref(that.ref)
  {
    if (ref)
    {
      IncRef(ref);
    }
  }

  shared_ref &operator= (const shared_ref &that) noexcept
  {
    auto copy(that);
    swap(copy);
    return *this;
  }

  shared_ref (shared_ref &&that) noexcept
    : ref(that.ref)
  {
    that.ref = null;
  }

  shared_ref &operator= (shared_ref &&that) noexcept
  {
    auto tmp(std::move(that));
    swap(tmp);
    return *this;
  }

  ~shared_ref () noexcept
  {
    if (ref)
    {
      DecRef(ref);
    }
  }

  void reset (T that = null) noexcept
  {
    auto tmp(std::move(*this));
    ref = that;
  }

  T release () noexcept
  {
    auto result = ref;
    ref = null;
    return result;
  }

  void swap (shared_ref &that) noexcept
  {
    using std::swap;
    swap(ref, that.ref);
  }

  bool is_null () const noexcept
  {
    return ref == null;
  }

  explicit operator bool () const noexcept
  {
    return ref != null;
  }
};


template <typename T,
  void(*DecRef)(__bits::native_ref<T>) = __bits::dec_ref<T>
>
struct unique_ref
{
  static constexpr T null{};
  T ref = null;

  unique_ref () = default;

  unique_ref (T ref) noexcept
    : ref(ref)
  {}

  unique_ref (const unique_ref &that) noexcept = delete;
  unique_ref &operator= (const unique_ref &that) noexcept = delete;

  unique_ref (unique_ref &&that) noexcept
    : ref(that.ref)
  {
    that.ref = null;
  }

  unique_ref &operator= (unique_ref &&that) noexcept
  {
    auto tmp(std::move(that));
    swap(tmp);
    return *this;
  }

  ~unique_ref () noexcept
  {
    if (ref)
    {
      DecRef(ref);
    }
  }

  void reset (T that = null) noexcept
  {
    auto tmp(std::move(*this));
    ref = that;
  }

  T release () noexcept
  {
    auto result = ref;
    ref = null;
    return result;
  }

  void swap (unique_ref &that) noexcept
  {
    using std::swap;
    swap(ref, that.ref);
  }

  bool is_null () const noexcept
  {
    return ref == null;
  }

  explicit operator bool () const noexcept
  {
    return ref != null;
  }
};


__sal_end
