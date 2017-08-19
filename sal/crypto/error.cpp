#include <sal/__bits/platform_sdk.hpp>
#include <sal/crypto/error.hpp>

#if __sal_os_darwin // {{{1
  #include <sal/__bits/ref.hpp>
  #include <Security/SecBase.h>
  #include <CoreFoundation/CFString.h>
#elif __sal_os_linux // {{{1
  #include <openssl/err.h>
#endif // }}}1


__sal_begin


namespace crypto {


namespace {

#if __sal_os_darwin // {{{1

struct category_impl_t
  : public std::error_category
{
  const char *name () const noexcept final override
  {
    return "crypto";
  }

  std::string message (int value) const final override
  {
    if (unique_ref<CFStringRef> s = ::SecCopyErrorMessageString(value, nullptr))
    {
      static constexpr auto encoding = kCFStringEncodingUTF8;

      if (auto p = ::CFStringGetCStringPtr(s.ref, encoding))
      {
        return p;
      }
      else
      {
        char buf[256];
        ::CFStringGetCString(s.ref, buf, sizeof(buf), encoding);
        return buf;
      }
    }

    std::string result = name();
    result += ':';
    result += std::to_string(value);
    return result;
  }
};

#elif __sal_os_linux // {{{1

struct category_impl_t
  : public std::error_category
{
  category_impl_t () noexcept
  {
    ERR_load_crypto_strings();
  }

  const char *name () const noexcept final override
  {
    return "crypto";
  }

  std::string message (int value) const final override
  {
    char buf[120];
    return ERR_error_string(value, buf);
  }
};

#elif __sal_os_windows // {{{1

#endif // }}}1

} // namespace


const std::error_category &category () noexcept
{
  #if __sal_os_darwin || __sal_os_linux
    static const category_impl_t cat_{};
    return cat_;
  #elif __sal_os_windows
    return std::system_category();
  #endif
}


} // namespace crypto


__sal_end
