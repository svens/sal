#pragma once

#include <sal/config.hpp>

#if __sal_os_darwin
  #include <CommonCrypto/CommonDigest.h>
  #include <CommonCrypto/CommonHMAC.h>
#elif __sal_os_linux
  #include <openssl/hmac.h>
  #include <openssl/md5.h>
  #include <openssl/sha.h>
  #include <memory>
#elif __sal_os_windows
  #include <windows.h>
  #include <bcrypt.h>
  #include <algorithm>
#else
  #error Unsupported platform
#endif


__sal_begin


namespace crypto { namespace __bits {


#if __sal_os_darwin //{{{1

  struct md5_t
  {
    using hash_t = CC_MD5_CTX;
    using hmac_t = CCHmacContext[2];
  };

  struct sha1_t
  {
    using hash_t = CC_SHA1_CTX;
    using hmac_t = CCHmacContext[2];
  };

  struct sha256_t
  {
    using hash_t = CC_SHA256_CTX;
    using hmac_t = CCHmacContext[2];
  };

  struct sha384_t
  {
    using hash_t = CC_SHA512_CTX;
    using hmac_t = CCHmacContext[2];
  };

  struct sha512_t
  {
    using hash_t = CC_SHA512_CTX;
    using hmac_t = CCHmacContext[2];
  };

#elif __sal_os_linux //{{{1

  struct hmac_ctx_t
  {
    std::unique_ptr<HMAC_CTX> ctx{};

    hmac_ctx_t (const EVP_MD *evp, const void *key, size_t size) noexcept;
    ~hmac_ctx_t () noexcept;

    hmac_ctx_t (const hmac_ctx_t &that);

    hmac_ctx_t (hmac_ctx_t &&that) noexcept
    {
      swap(*this, that);
    }

    hmac_ctx_t &operator= (hmac_ctx_t that) noexcept
    {
      swap(*this, that);
      return *this;
    }

    friend void swap (hmac_ctx_t &a, hmac_ctx_t &b) noexcept
    {
      using std::swap;
      swap(a.ctx, b.ctx);
    }

    void update (const void *data, size_t size);
    void finish (void *result);

    static void one_shot (const EVP_MD *evp,
      const void *key, size_t key_size,
      const void *data, size_t data_size,
      void *result
    );
  };

  struct md5_t
  {
    using hash_t = MD5_CTX;
    using hmac_t = hmac_ctx_t;
  };

  struct sha1_t
  {
    using hash_t = SHA_CTX;
    using hmac_t = hmac_ctx_t;
  };

  struct sha256_t
  {
    using hash_t = SHA256_CTX;
    using hmac_t = hmac_ctx_t;
  };

  struct sha384_t
  {
    using hash_t = SHA512_CTX;
    using hmac_t = hmac_ctx_t;
  };

  struct sha512_t
  {
    using hash_t = SHA512_CTX;
    using hmac_t = hmac_ctx_t;
  };

#elif __sal_os_windows //{{{1

  struct digest_t
  {
    BCRYPT_HASH_HANDLE handle{};

    digest_t () noexcept = default;
    ~digest_t () noexcept;

    digest_t (BCRYPT_HASH_HANDLE handle) noexcept
      : handle(handle)
    {}

    digest_t (const digest_t &that);

    digest_t (digest_t &&that) noexcept
    {
      swap(*this, that);
    }

    digest_t &operator= (digest_t that) noexcept
    {
      swap(*this, that);
      return *this;
    }

    friend void swap (digest_t &a, digest_t &b) noexcept
    {
      using std::swap;
      swap(a.handle, b.handle);
    }

    template <typename Digest, bool IsHMAC>
    static BCRYPT_ALG_HANDLE factory ();

    template <typename Digest, bool IsHMAC>
    static BCRYPT_HASH_HANDLE make (const void *key = nullptr, size_t = 0U);

    void update (const void *data, size_t size);
    void finish (void *result, size_t size);

    static void hash (BCRYPT_ALG_HANDLE algorithm,
      const void *data, size_t data_size,
      void *result, size_t result_size
    );

    static void hmac (BCRYPT_ALG_HANDLE algorithm,
      const void *key, size_t key_size,
      const void *data, size_t data_size,
      void *result, size_t result_size
    );
  };

  struct md5_t
  {
    using hash_t = digest_t;
    using hmac_t = digest_t;
  };

  struct sha1_t
  {
    using hash_t = digest_t;
    using hmac_t = digest_t;
  };

  struct sha256_t
  {
    using hash_t = digest_t;
    using hmac_t = digest_t;
  };

  struct sha384_t
  {
    using hash_t = digest_t;
    using hmac_t = digest_t;
  };

  struct sha512_t
  {
    using hash_t = digest_t;
    using hmac_t = digest_t;
  };

#endif //}}}1


template <typename Digest> constexpr size_t digest_size_v = 0U;
template <> constexpr size_t digest_size_v<md5_t> = 16U;
template <> constexpr size_t digest_size_v<sha1_t> = 20U;
template <> constexpr size_t digest_size_v<sha256_t> = 32U;
template <> constexpr size_t digest_size_v<sha384_t> = 48U;
template <> constexpr size_t digest_size_v<sha512_t> = 64U;


}} // namespace crypto::__bits


__sal_end
