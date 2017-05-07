#pragma once

#include <sal/config.hpp>

#if __sal_os_darwin
  #include <CommonCrypto/CommonDigest.h>
  #include <CommonCrypto/CommonHMAC.h>
#elif __sal_os_linux
  // everything in .cpp
#elif __sal_os_windows
  // everything in .cpp
#else
  #error Unsupported platform
#endif


__sal_begin


namespace crypto { namespace __bits {


#if __sal_os_darwin

  template <typename Context>
  struct basic_hash_t
    : protected Context
  {
    basic_hash_t () noexcept = default;

    basic_hash_t (const basic_hash_t &) noexcept = delete;
    basic_hash_t &operator= (const basic_hash_t &) noexcept = delete;

    basic_hash_t (basic_hash_t &&) noexcept = default;
    basic_hash_t &operator= (basic_hash_t &&) noexcept = default;

    ~basic_hash_t () noexcept = default;
  };

  using md5_hash_t = basic_hash_t<CC_MD5_CTX>;
  using sha1_hash_t = basic_hash_t<CC_SHA1_CTX>;
  using sha256_hash_t = basic_hash_t<CC_SHA256_CTX>;
  using sha384_hash_t = basic_hash_t<CC_SHA512_CTX>;
  using sha512_hash_t = basic_hash_t<CC_SHA512_CTX>;

  struct basic_hmac_t
  {
    CCHmacContext current{}, original{};

    basic_hmac_t () noexcept = default;

    basic_hmac_t (const basic_hmac_t &) noexcept = delete;
    basic_hmac_t &operator= (const basic_hmac_t &) noexcept = delete;

    basic_hmac_t (basic_hmac_t &&) noexcept = default;
    basic_hmac_t &operator= (basic_hmac_t &&) noexcept = default;

    ~basic_hmac_t () noexcept = default;
  };

  using md5_hmac_t = basic_hmac_t;
  using sha1_hmac_t = basic_hmac_t;
  using sha256_hmac_t = basic_hmac_t;
  using sha384_hmac_t = basic_hmac_t;
  using sha512_hmac_t = basic_hmac_t;

#elif __sal_os_linux

  using md5_hash_t = int;
  using sha1_hash_t = int;
  using sha256_hash_t = int;
  using sha384_hash_t = int;
  using sha512_hash_t = int;

#elif __sal_os_windows

  struct basic_hash_t
  {
    uintptr_t handle;

    basic_hash_t (uintptr_t handle) noexcept
      : handle(handle)
    {}

    basic_hash_t (const basic_hash_t &) noexcept = delete;
    basic_hash_t &operator= (const basic_hash_t &) noexcept = delete;

    basic_hash_t (basic_hash_t &&) noexcept;
    basic_hash_t &operator= (basic_hash_t &&) noexcept;

    ~basic_hash_t () noexcept;

    void update (const void *data, size_t size);
    void finish (void *result, size_t size);
  };

  using md5_hash_t = basic_hash_t;
  using sha1_hash_t = basic_hash_t;
  using sha256_hash_t = basic_hash_t;
  using sha384_hash_t = basic_hash_t;
  using sha512_hash_t = basic_hash_t;

  using basic_hmac_t = basic_hash_t;
  using md5_hmac_t = basic_hmac_t;
  using sha1_hmac_t = basic_hmac_t;
  using sha256_hmac_t = basic_hmac_t;
  using sha384_hmac_t = basic_hmac_t;
  using sha512_hmac_t = basic_hmac_t;

#endif


struct md5_t // {{{1
{
  static constexpr size_t digest_size = 16U;


  struct hash_t
    : protected md5_hash_t
  {
    hash_t ();
    void update (const void *data, size_t size);
    void finish (void *result);
  };


  struct hmac_t
    : protected md5_hmac_t
  {
    hmac_t ()
      : hmac_t(nullptr, 0U)
    {}

    hmac_t (const void *key, size_t length);

    void update (const void *data, size_t size);
    void finish (void *result);
  };
};


struct sha1_t // {{{1
{
  static constexpr size_t digest_size = 20U;


  struct hash_t
    : protected sha1_hash_t
  {
    hash_t ();
    void update (const void *data, size_t size);
    void finish (void *result);
  };


  struct hmac_t
    : protected sha1_hmac_t
  {
    hmac_t ()
      : hmac_t(nullptr, 0U)
    {}

    hmac_t (const void *key, size_t length);

    void update (const void *data, size_t size);
    void finish (void *result);
  };
};


struct sha256_t // {{{1
{
  static constexpr size_t digest_size = 32U;


  struct hash_t
    : protected sha256_hash_t
  {
    hash_t ();
    void update (const void *data, size_t size);
    void finish (void *result);
  };


  struct hmac_t
    : protected sha256_hmac_t
  {
    hmac_t ()
      : hmac_t(nullptr, 0U)
    {}

    hmac_t (const void *key, size_t length);

    void update (const void *data, size_t size);
    void finish (void *result);
  };
};


struct sha384_t // {{{1
{
  static constexpr size_t digest_size = 48U;


  struct hash_t
    : protected sha384_hash_t
  {
    hash_t ();
    void update (const void *data, size_t size);
    void finish (void *result);
  };


  struct hmac_t
    : protected sha384_hmac_t
  {
    hmac_t ()
      : hmac_t(nullptr, 0U)
    {}

    hmac_t (const void *key, size_t length);

    void update (const void *data, size_t size);
    void finish (void *result);
  };
};


struct sha512_t // {{{1
{
  static constexpr size_t digest_size = 64U;


  struct hash_t
    : protected sha512_hash_t
  {
    hash_t ();
    void update (const void *data, size_t size);
    void finish (void *result);
  };


  struct hmac_t
    : protected sha512_hmac_t
  {
    hmac_t ()
      : hmac_t(nullptr, 0U)
    {}

    hmac_t (const void *key, size_t length);

    void update (const void *data, size_t size);
    void finish (void *result);
  };
};


}} // namespace crypto::__bits


__sal_end
