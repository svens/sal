#pragma once

#if defined(__sal_begin)
  #error This header must be included first
#endif

#if defined(_WIN32) || defined(_WIN64) // {{{1
  #define WIN32_NO_STATUS
  #include <windows.h>
  #undef WIN32_NO_STATUS
  #include <winternl.h>
  #include <ntstatus.h>
  #pragma comment(lib, "ntdll")
#endif // }}}1


#if defined(__APPLE__) // {{{1
  #include <sal/config.hpp>
  #include <CoreFoundation/CFBase.h>
  #include <utility>

  __sal_begin

  template <typename T>
  struct scoped_ref
  {
    T ref;

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

    scoped_ref (const scoped_ref &) = delete;
    scoped_ref &operator= (const scoped_ref &) = delete;

    void swap (scoped_ref &that) noexcept
    {
      using std::swap;
      swap(ref, that.ref);
    }

    explicit operator bool () const noexcept
    {
      return ref != nullptr;
    }
  };

  __sal_end

#endif // }}}1
