#pragma once

// DO NOT INCLUDE DIRECTLY
// It is incuded from sal/crypto_hash.hpp


#include <sal/config.hpp>
#if __sal_os_darwin
  #include <CommonCrypto/CommonDigest.h>
#elif __sal_os_windows
  /* Everything in .cpp */
#else
  #error Unsupported platform
#endif


__sal_begin


namespace crypto { namespace __bits {


#if __sal_os_darwin


class md2_t
{
public:

  md2_t ()
  {
    CC_MD2_Init(&ctx_);
  }

  static constexpr size_t size () noexcept
  {
    return CC_MD2_DIGEST_LENGTH;
  }

  template <typename Ptr>
  void add (const Ptr &ptr) noexcept
  {
    CC_MD2_Update(&ctx_, ptr.data(), ptr.size());
  }

  template <typename Ptr>
  void finish (Ptr &ptr) noexcept
  {
    CC_MD2_Final(ptr.data(), &ctx_);
    CC_MD2_Init(&ctx_);
  }


private:

  CC_MD2_CTX ctx_{};
};


class md4_t
{
public:

  md4_t ()
  {
    CC_MD4_Init(&ctx_);
  }

  static constexpr size_t size () noexcept
  {
    return CC_MD4_DIGEST_LENGTH;
  }

  template <typename Ptr>
  void add (const Ptr &ptr) noexcept
  {
    CC_MD4_Update(&ctx_, ptr.data(), ptr.size());
  }

  template <typename Ptr>
  void finish (Ptr &ptr) noexcept
  {
    CC_MD4_Final(ptr.data(), &ctx_);
    CC_MD4_Init(&ctx_);
  }


private:

  CC_MD4_CTX ctx_{};
};


class md5_t
{
public:

  md5_t ()
  {
    CC_MD5_Init(&ctx_);
  }

  static constexpr size_t size () noexcept
  {
    return CC_MD5_DIGEST_LENGTH;
  }

  template <typename Ptr>
  void add (const Ptr &ptr) noexcept
  {
    CC_MD5_Update(&ctx_, ptr.data(), ptr.size());
  }

  template <typename Ptr>
  void finish (Ptr &ptr) noexcept
  {
    CC_MD5_Final(ptr.data(), &ctx_);
    CC_MD5_Init(&ctx_);
  }


private:

  CC_MD5_CTX ctx_{};
};


class sha_1_t
{
public:

  sha_1_t ()
  {
    CC_SHA1_Init(&ctx_);
  }

  static constexpr size_t size () noexcept
  {
    return CC_SHA1_DIGEST_LENGTH;
  }

  template <typename Ptr>
  void add (const Ptr &ptr) noexcept
  {
    CC_SHA1_Update(&ctx_, ptr.data(), ptr.size());
  }

  template <typename Ptr>
  void finish (Ptr &ptr) noexcept
  {
    CC_SHA1_Final(ptr.data(), &ctx_);
    CC_SHA1_Init(&ctx_);
  }


private:

  CC_SHA1_CTX ctx_{};
};


class sha_256_t
{
public:

  sha_256_t ()
  {
    CC_SHA256_Init(&ctx_);
  }

  static constexpr size_t size () noexcept
  {
    return CC_SHA256_DIGEST_LENGTH;
  }

  template <typename Ptr>
  void add (const Ptr &ptr) noexcept
  {
    CC_SHA256_Update(&ctx_, ptr.data(), ptr.size());
  }

  template <typename Ptr>
  void finish (Ptr &ptr) noexcept
  {
    CC_SHA256_Final(ptr.data(), &ctx_);
    CC_SHA256_Init(&ctx_);
  }


private:

  CC_SHA256_CTX ctx_{};
};


class sha_384_t
{
public:

  sha_384_t ()
  {
    CC_SHA384_Init(&ctx_);
  }

  static constexpr size_t size () noexcept
  {
    return CC_SHA384_DIGEST_LENGTH;
  }

  template <typename Ptr>
  void add (const Ptr &ptr) noexcept
  {
    CC_SHA384_Update(&ctx_, ptr.data(), ptr.size());
  }

  template <typename Ptr>
  void finish (Ptr &ptr) noexcept
  {
    CC_SHA384_Final(ptr.data(), &ctx_);
    CC_SHA384_Init(&ctx_);
  }


private:

  CC_SHA512_CTX ctx_{};
};


class sha_512_t
{
public:

  sha_512_t ()
  {
    CC_SHA512_Init(&ctx_);
  }

  static constexpr size_t size () noexcept
  {
    return CC_SHA512_DIGEST_LENGTH;
  }

  template <typename Ptr>
  void add (const Ptr &ptr) noexcept
  {
    CC_SHA512_Update(&ctx_, ptr.data(), ptr.size());
  }

  template <typename Ptr>
  void finish (Ptr &ptr) noexcept
  {
    CC_SHA512_Final(ptr.data(), &ctx_);
    CC_SHA512_Init(&ctx_);
  }


private:

  CC_SHA512_CTX ctx_{};
};


#elif __sal_os_windows


class basic_hash_t
{
public:

  template <typename Ptr>
  void add (const Ptr &ptr) noexcept
  {
    add(ptr.data(), ptr.size());
  }

  template <typename Ptr>
  void finish (Ptr &ptr) noexcept
  {
    finish(ptr.data(), ptr.size());
  }

protected:

  uintptr_t handle_;

  basic_hash_t (uintptr_t handle) noexcept
    : handle_(handle)
  {}

  ~basic_hash_t () noexcept;

  void add (const void *data, size_t lenght);
  void finish (const void *data, size_t lenght);
};


class md2_t
  : public basic_hash_t
{
public:

  md2_t ();

  static constexpr size_t size () noexcept
  {
    return 16U;
  }
};


class md4_t
  : public basic_hash_t
{
public:

  md4_t ();

  static constexpr size_t size () noexcept
  {
    return 16U;
  }
};


class md5_t
  : public basic_hash_t
{
public:

  md5_t ();

  static constexpr size_t size () noexcept
  {
    return 16U;
  }
};


class sha_1_t
  : public basic_hash_t
{
public:

  sha_1_t ();

  static constexpr size_t size () noexcept
  {
    return 20U;
  }
};


class sha_256_t
  : public basic_hash_t
{
public:

  sha_256_t ();

  static constexpr size_t size () noexcept
  {
    return 32U;
  }
};


class sha_384_t
  : public basic_hash_t
{
public:

  sha_384_t ();

  static constexpr size_t size () noexcept
  {
    return 48U;
  }
};


class sha_512_t
  : public basic_hash_t
{
public:

  sha_512_t ();

  static constexpr size_t size () noexcept
  {
    return 64U;
  }
};


#endif


}} // namespace crypto::__bits


__sal_end
