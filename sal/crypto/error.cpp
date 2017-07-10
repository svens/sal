#include <sal/__bits/platform_sdk.hpp>
#include <sal/crypto/error.hpp>

#if __sal_os_darwin // {{{1
  #include <Security/SecBase.h>
  #include <CoreFoundation/CFString.h>
#endif // }}}1


__sal_begin


namespace crypto {


namespace {

#if __sal_os_darwin // {{{1

class category_impl_t
  : public std::error_category
{
  const char *name () const noexcept final override
  {
    return "crypto";
  }

  std::string message (int value) const final override
  {
    if (scoped_ref<CFStringRef> s = ::SecCopyErrorMessageString(value, nullptr))
    {
      static constexpr auto encoding = kCFStringEncodingUTF8;

      auto size = ::CFStringGetLength(s.ref);
      std::string result;

      if (auto p = ::CFStringGetCStringPtr(s.ref, encoding))
      {
        result.assign(p, size);
      }
      else
      {
        result.resize(size);
        ::CFStringGetCString(s.ref,
          const_cast<char *>(result.data()),
          result.size(),
          encoding
        );
      }

      return result;
    }

    return "Unknown error " + std::to_string(value);
  }
};

#elif __sal_os_linux // {{{1

#elif __sal_os_windows // {{{1

#endif // }}}1

} // namespace


const std::error_category &category () noexcept
{
  #if __sal_os_darwin
    static const category_impl_t cat_{};
    return cat_;
  #elif __sal_os_linux
    return std::generic_category();
  #elif __sal_os_windows
    return std::system_category();
  #endif
}


} // namespace crypto


__sal_end
