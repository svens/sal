#pragma once

#include <sal/config.hpp>

#if __sal_os_darwin
  #include <CommonCrypto/CommonDigest.h>
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

  using md5_ctx = CC_MD5_CTX;
  using sha1_ctx = CC_SHA1_CTX;
  using sha256_ctx = CC_SHA256_CTX;
  using sha384_ctx = CC_SHA512_CTX;
  using sha512_ctx = CC_SHA512_CTX;

#elif __sal_os_linux

  using md5_ctx = int;
  using sha1_ctx = int;
  using sha256_ctx = int;
  using sha384_ctx = int;
  using sha512_ctx = int;

#elif __sal_os_windows

  using md5_ctx = uintptr_t;
  using sha1_ctx = uintptr_t;
  using sha256_ctx = uintptr_t;
  using sha384_ctx = uintptr_t;
  using sha512_ctx = uintptr_t;

#endif


struct md5_t // {{{1
{
  static constexpr size_t digest_size = 16U;
  struct hash_t;
};


struct md5_t::hash_t
{
  md5_ctx ctx{};

  hash_t ();
  ~hash_t () noexcept;

  hash_t (hash_t &&that) noexcept;
  hash_t &operator= (hash_t &&that) noexcept;

  hash_t (const hash_t &) = delete;
  hash_t &operator= (const hash_t &) = delete;

  void update (const void *data, size_t size);
  void finish (void *result);
};


struct sha1_t // {{{1
{
  static constexpr size_t digest_size = 20U;
  struct hash_t;
};


struct sha1_t::hash_t
{
  sha1_ctx ctx{};

  hash_t ();
  ~hash_t () noexcept;

  hash_t (hash_t &&that) noexcept;
  hash_t &operator= (hash_t &&that) noexcept;

  hash_t (const hash_t &) = delete;
  hash_t &operator= (const hash_t &) = delete;

  void update (const void *data, size_t size);
  void finish (void *result);
};


struct sha256_t // {{{1
{
  static constexpr size_t digest_size = 32U;
  struct hash_t;
};


struct sha256_t::hash_t
{
  sha256_ctx ctx{};

  hash_t ();
  ~hash_t () noexcept;

  hash_t (hash_t &&that) noexcept;
  hash_t &operator= (hash_t &&that) noexcept;

  hash_t (const hash_t &) = delete;
  hash_t &operator= (const hash_t &) = delete;

  void update (const void *data, size_t size);
  void finish (void *result);
};


struct sha384_t // {{{1
{
  static constexpr size_t digest_size = 48U;
  struct hash_t;
};


struct sha384_t::hash_t
{
  sha384_ctx ctx{};

  hash_t ();
  ~hash_t () noexcept;

  hash_t (hash_t &&that) noexcept;
  hash_t &operator= (hash_t &&that) noexcept;

  hash_t (const hash_t &) = delete;
  hash_t &operator= (const hash_t &) = delete;

  void update (const void *data, size_t size);
  void finish (void *result);
};


struct sha512_t // {{{1
{
  static constexpr size_t digest_size = 64U;
  struct hash_t;
};


struct sha512_t::hash_t
{
  sha512_ctx ctx{};

  hash_t ();
  ~hash_t () noexcept;

  hash_t (hash_t &&that) noexcept;
  hash_t &operator= (hash_t &&that) noexcept;

  hash_t (const hash_t &) = delete;
  hash_t &operator= (const hash_t &) = delete;

  void update (const void *data, size_t size);
  void finish (void *result);
};


}} // namespace crypto::__bits


__sal_end
