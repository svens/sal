#pragma once

#include <sal/config.hpp>
#include <sal/__bits/ref.hpp>

#if __sal_os_darwin //{{{1
  #if !defined(__apple_build_version__)
    #define availability(...) /**/
  #endif
  #include <Security/SecCertificate.h>
  #include <Security/SecKey.h>
  #undef availability
#elif __sal_os_linux //{{{1
  #include <openssl/evp.h>
  #include <openssl/x509.h>
#elif __sal_os_windows //{{{1
  #include <windows.h>
  #include <wincrypt.h>
  #pragma comment(lib, "crypt32")
#endif //}}}1


__sal_begin


namespace crypto { namespace __bits {


#if __sal_os_darwin //{{{1

using certificate_t = shared_ref<SecCertificateRef>;
using public_key_t = unique_ref<SecKeyRef>;
using private_key_t = unique_ref<SecKeyRef>;

#elif __sal_os_linux //{{{1

inline X509 *inc_ref (X509 *ref) noexcept
{
#if OPENSSL_VERSION_NUMBER < 0x10100000
  CRYPTO_add(&ref->references, 1, CRYPTO_LOCK_X509);
#else
  X509_up_ref(ref);
#endif
  return ref;
}

inline void dec_ref (X509 *ref) noexcept
{
  X509_free(ref);
}

using certificate_t = shared_ref<X509 *, inc_ref, dec_ref>;

inline void dec_ref (EVP_PKEY *ref) noexcept
{
  EVP_PKEY_free(ref);
}

using public_key_t = unique_ref<EVP_PKEY *, dec_ref>;

#elif __sal_os_windows //{{{1

inline PCCERT_CONTEXT inc_ref (PCCERT_CONTEXT ref) noexcept
{
  return ::CertDuplicateCertificateContext(ref);
}

inline void dec_ref (PCCERT_CONTEXT ref) noexcept
{
  (void)::CertFreeCertificateContext(ref);
}

using certificate_t = shared_ref<PCCERT_CONTEXT, inc_ref, dec_ref>;

inline void dec_ref (BCRYPT_KEY_HANDLE ref) noexcept
{
  (void)::BCryptDestroyKey(ref);
}

using public_key_t = unique_ref<BCRYPT_KEY_HANDLE, dec_ref>;

#endif //}}}1


//
// Helpers
//

uint8_t *pem_to_der (
  const uint8_t *pem_first, const uint8_t *pem_last,
  uint8_t *der_first, uint8_t *der_last
) noexcept;


}} // namespace crypto::__bits


__sal_end
