#if defined(_WIN32) || defined(_WIN64)
  #define WIN32_NO_STATUS
  #include <windows.h>
  #include <bcrypt.h>
  #undef WIN32_NO_STATUS
  #include <winternl.h>
  #include <ntstatus.h>
  #pragma comment(lib, "bcrypt")
#endif

#include <sal/crypto/random.hpp>
#include <sal/error.hpp>

#if __sal_os_macos
  #include <CommonCrypto/CommonCrypto.h>
  #include <CommonCrypto/CommonRandom.h>
#elif __sal_os_linux
  #include <openssl/rand.h>
#elif __sal_os_windows
  // all included above
#else
  #error Unsupported platform
#endif


__sal_begin


namespace crypto::__bits {


void random (void *data, size_t size)
{
  bool random_failed = !data || !size;

  if (!random_failed)
  {
#if __sal_os_macos
    random_failed = kCCSuccess != ::CCRandomGenerateBytes(data, size);
#elif __sal_os_linux
    random_failed = !RAND_bytes(static_cast<uint8_t *>(data), size);
#elif __sal_os_windows
    random_failed = STATUS_SUCCESS != ::BCryptGenRandom(nullptr,
      static_cast<PUCHAR>(data),
      static_cast<ULONG>(size),
      BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
#endif
  }

  sal_throw_if(random_failed);
}


} // namespace crypto::__bits


__sal_end
