#pragma once

#include <sal/config.hpp>

#if __sal_os_darwin
  #include <CommonCrypto/CommonDigest.h>
#elif __sal_os_linux
  // TODO
#elif __sal_os_windows
  // everything in .cpp
#else
  #error Unsupported platform
#endif


__sal_begin


namespace crypto { namespace __bits {


#if __sal_os_darwin

  using md2_ctx = CC_MD2_CTX;
  using md4_ctx = CC_MD4_CTX;
  using md5_ctx = CC_MD5_CTX;
  using sha1_ctx = CC_SHA1_CTX;
  using sha256_ctx = CC_SHA256_CTX;
  using sha384_ctx = CC_SHA512_CTX;
  using sha512_ctx = CC_SHA512_CTX;

#elif __sal_os_linux

  using md2_ctx = uintptr_t;
  using md4_ctx = uintptr_t;
  using md5_ctx = uintptr_t;
  using sha1_ctx = uintptr_t;
  using sha256_ctx = uintptr_t;
  using sha384_ctx = uintptr_t;
  using sha512_ctx = uintptr_t;

#elif __sal_os_windows

  using md2_ctx = uintptr_t;
  using md4_ctx = uintptr_t;
  using md5_ctx = uintptr_t;
  using sha1_ctx = uintptr_t;
  using sha256_ctx = uintptr_t;
  using sha384_ctx = uintptr_t;
  using sha512_ctx = uintptr_t;

#endif


struct md2_t // {{{1
{
  static constexpr size_t digest_size = 16U;
  struct hash_t;
};


struct md2_t::hash_t
{
  md2_ctx ctx{};

  hash_t ();
  ~hash_t () noexcept;
  void add (const void *data, size_t size) noexcept;
  void finish (void *result) noexcept;
};


struct md4_t // {{{1
{
  static constexpr size_t digest_size = 16U;
  struct hash_t;
};


struct md4_t::hash_t
{
  md4_ctx ctx{};

  hash_t ();
  ~hash_t () noexcept;
  void add (const void *data, size_t size) noexcept;
  void finish (void *result) noexcept;
};


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
  void add (const void *data, size_t size) noexcept;
  void finish (void *result) noexcept;
};


struct sha_1_t // {{{1
{
  static constexpr size_t digest_size = 20U;
  struct hash_t;
};


struct sha_1_t::hash_t
{
  sha1_ctx ctx{};

  hash_t ();
  ~hash_t () noexcept;
  void add (const void *data, size_t size) noexcept;
  void finish (void *result) noexcept;
};


struct sha_256_t // {{{1
{
  static constexpr size_t digest_size = 32U;
  struct hash_t;
};


struct sha_256_t::hash_t
{
  sha256_ctx ctx{};

  hash_t ();
  ~hash_t () noexcept;
  void add (const void *data, size_t size) noexcept;
  void finish (void *result) noexcept;
};


struct sha_384_t // {{{1
{
  static constexpr size_t digest_size = 48U;
  struct hash_t;
};


struct sha_384_t::hash_t
{
  sha384_ctx ctx{};

  hash_t ();
  ~hash_t () noexcept;
  void add (const void *data, size_t size) noexcept;
  void finish (void *result) noexcept;
};


struct sha_512_t // {{{1
{
  static constexpr size_t digest_size = 64U;
  struct hash_t;
};


struct sha_512_t::hash_t
{
  sha512_ctx ctx{};

  hash_t ();
  ~hash_t () noexcept;
  void add (const void *data, size_t size) noexcept;
  void finish (void *result) noexcept;
};


}} // namespace crypto::__bits


__sal_end
