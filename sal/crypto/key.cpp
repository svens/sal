#include <sal/crypto/key.hpp>

#if 0
#if __sal_os_macos //{{{1
  #if !defined(__apple_build_version__)
    #define availability(...) /**/
  #endif
  #include <CoreFoundation/CFNumber.h>
  #include <CoreFoundation/CFURL.h>
  #include <Security/SecCertificateOIDs.h>
#elif __sal_os_linux //{{{1
  #include <openssl/asn1.h>
  #include <openssl/x509v3.h>
#endif //}}}1
#endif


__sal_begin


namespace crypto {


#if __sal_os_macos // {{{1

#elif __sal_os_linux //{{{1

#elif __sal_os_windows // {{{1

#endif // }}}1


} // namespace crypto


__sal_end
