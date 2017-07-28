#pragma once

#include <sal/config.hpp>
#include <sal/__bits/scoped_ref.hpp>

#if __sal_os_darwin //{{{1
  #include <Security/SecCertificate.h>
#elif __sal_os_windows //{{{1
  #include <windows.h>
  #include <wincrypt.h>
  #pragma comment(lib, "crypt32")
#endif //}}}1


__sal_begin


namespace crypto { namespace __bits {


#if __sal_os_darwin // {{{1

using certificate_t = scoped_ref<SecCertificateRef>;

#elif __sal_os_windows // {{{1

inline PCCERT_CONTEXT inc_ref (PCCERT_CONTEXT ref) noexcept
{
  return ::CertDuplicateCertificateContext(ref);
}

inline void dec_ref (PCCERT_CONTEXT ref) noexcept
{
  (void)::CertFreeCertificateContext(ref);
}

using certificate_t = scoped_ref<PCCERT_CONTEXT, inc_ref, dec_ref>;

#endif // }}}1


}} // namespace crypto::__bits


__sal_end
