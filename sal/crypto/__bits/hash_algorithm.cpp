#if defined(_WIN32) || defined(_WIN64)
  #define WIN32_NO_STATUS
  #include <windows.h>
  #include <bcrypt.h>
  #undef WIN32_NO_STATUS
  #include <winternl.h>
  #include <ntstatus.h>
  #pragma comment(lib, "ncrypt")
  #pragma comment(lib, "ntdll")
#endif

#include <sal/crypto/__bits/hash_algorithm.hpp>
#include <sal/assert.hpp>
#include <sal/error.hpp>


__sal_begin

namespace crypto { namespace __bits {


#if __sal_os_darwin // {{{1


md2_t::hash_t::hash_t () // {{{2
{
  CC_MD2_Init(&ctx);
}


md2_t::hash_t::~hash_t () noexcept
{}


void md2_t::hash_t::add (const void *data, size_t size) noexcept
{
  CC_MD2_Update(&ctx, data, size);
}


void md2_t::hash_t::finish (void *result) noexcept
{
  CC_MD2_Final(static_cast<uint8_t *>(result), &ctx);
  CC_MD2_Init(&ctx);
}


md4_t::hash_t::hash_t () // {{{2
{
  CC_MD4_Init(&ctx);
}


md4_t::hash_t::~hash_t () noexcept
{}


void md4_t::hash_t::add (const void *data, size_t size) noexcept
{
  CC_MD4_Update(&ctx, data, size);
}


void md4_t::hash_t::finish (void *result) noexcept
{
  CC_MD4_Final(static_cast<uint8_t *>(result), &ctx);
  CC_MD4_Init(&ctx);
}


md5_t::hash_t::hash_t () // {{{2
{
  CC_MD5_Init(&ctx);
}


md5_t::hash_t::~hash_t () noexcept
{}


void md5_t::hash_t::add (const void *data, size_t size) noexcept
{
  CC_MD5_Update(&ctx, data, size);
}


void md5_t::hash_t::finish (void *result) noexcept
{
  CC_MD5_Final(static_cast<uint8_t *>(result), &ctx);
  CC_MD5_Init(&ctx);
}


sha_1_t::hash_t::hash_t () // {{{2
{
  CC_SHA1_Init(&ctx);
}


sha_1_t::hash_t::~hash_t () noexcept
{}


void sha_1_t::hash_t::add (const void *data, size_t size) noexcept
{
  CC_SHA1_Update(&ctx, data, size);
}


void sha_1_t::hash_t::finish (void *result) noexcept
{
  CC_SHA1_Final(static_cast<uint8_t *>(result), &ctx);
  CC_SHA1_Init(&ctx);
}


sha_256_t::hash_t::hash_t () // {{{2
{
  CC_SHA256_Init(&ctx);
}


sha_256_t::hash_t::~hash_t () noexcept
{}


void sha_256_t::hash_t::add (const void *data, size_t size) noexcept
{
  CC_SHA256_Update(&ctx, data, size);
}


void sha_256_t::hash_t::finish (void *result) noexcept
{
  CC_SHA256_Final(static_cast<uint8_t *>(result), &ctx);
  CC_SHA256_Init(&ctx);
}


sha_384_t::hash_t::hash_t () // {{{2
{
  CC_SHA384_Init(&ctx);
}


sha_384_t::hash_t::~hash_t () noexcept
{}


void sha_384_t::hash_t::add (const void *data, size_t size) noexcept
{
  CC_SHA384_Update(&ctx, data, size);
}


void sha_384_t::hash_t::finish (void *result) noexcept
{
  CC_SHA384_Final(static_cast<uint8_t *>(result), &ctx);
  CC_SHA384_Init(&ctx);
}


sha_512_t::hash_t::hash_t () // {{{2
{
  CC_SHA512_Init(&ctx);
}


sha_512_t::hash_t::~hash_t () noexcept
{}


void sha_512_t::hash_t::add (const void *data, size_t size) noexcept
{
  CC_SHA512_Update(&ctx, data, size);
}


void sha_512_t::hash_t::finish (void *result) noexcept
{
  CC_SHA512_Final(static_cast<uint8_t *>(result), &ctx);
  CC_SHA512_Init(&ctx);
}


#elif __sal_os_windows // {{{1


namespace {


inline void check_result (NTSTATUS status, const char *func)
{
  if (!NT_SUCCESS(status))
  {
    std::error_code error(
      ::RtlNtStatusToDosError(status),
      std::system_category()
    );
    throw_system_error(error, func);
  }
}
#define call(func,...) check_result(func(__VA_ARGS__), #func)


struct algorithm_provider_t
{
  BCRYPT_ALG_HANDLE provider;

  algorithm_provider_t (LPCWSTR id, DWORD flags, size_t expected_hash_length);

  ~algorithm_provider_t () noexcept
  {
    ::BCryptCloseAlgorithmProvider(provider, 0);
  }
};


algorithm_provider_t::algorithm_provider_t (LPCWSTR id,
  DWORD flags,
  size_t expected_hash_length)
{
  flags |= BCRYPT_HASH_REUSABLE_FLAG;
  call(::BCryptOpenAlgorithmProvider, &provider, id, nullptr, flags);

  DWORD hash_length, cbData;
  call(::BCryptGetProperty,
    provider,
    BCRYPT_HASH_LENGTH,
    reinterpret_cast<PBYTE>(&hash_length),
    sizeof(hash_length),
    &cbData,
    0
  );

  sal_assert(hash_length == expected_hash_length);
  (void)expected_hash_length;
}


template <typename Algorithm>
uintptr_t make_hash (LPCWSTR id)
{
  static algorithm_provider_t algorithm{id, 0, Algorithm::digest_size};

  BCRYPT_HASH_HANDLE hash_handle;
  call(::BCryptCreateHash,
    algorithm.provider,
    &hash_handle,
    nullptr, 0,
    nullptr, 0,
    BCRYPT_HASH_REUSABLE_FLAG
  );
  return reinterpret_cast<uintptr_t>(hash_handle);
}


void hash_release (uintptr_t handle) noexcept
{
  ::BCryptDestroyHash(reinterpret_cast<BCRYPT_HASH_HANDLE>(handle));
}


void hash_add (uintptr_t handle, const void *data, size_t size) noexcept
{
  call(::BCryptHashData,
    reinterpret_cast<BCRYPT_HASH_HANDLE>(handle),
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(size),
    0
  );
}


void hash_finish (uintptr_t handle, void *result, size_t size) noexcept
{
  call(::BCryptFinishHash,
    reinterpret_cast<BCRYPT_HASH_HANDLE>(handle),
    static_cast<PUCHAR>(result),
    static_cast<ULONG>(size),
    0
  );
}


} // namespace


md2_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<md2_t>(BCRYPT_MD2_ALGORITHM))
{}


md2_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void md2_t::hash_t::add (const void *data, size_t size) noexcept
{
  hash_add(ctx, data, size);
}


void md2_t::hash_t::finish (void *result) noexcept
{
  hash_finish(ctx, result, digest_size);
}


md4_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<md4_t>(BCRYPT_MD4_ALGORITHM))
{}


md4_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void md4_t::hash_t::add (const void *data, size_t size) noexcept
{
  hash_add(ctx, data, size);
}


void md4_t::hash_t::finish (void *result) noexcept
{
  hash_finish(ctx, result, digest_size);
}


md5_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<md5_t>(BCRYPT_MD5_ALGORITHM))
{}


md5_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void md5_t::hash_t::add (const void *data, size_t size) noexcept
{
  hash_add(ctx, data, size);
}


void md5_t::hash_t::finish (void *result) noexcept
{
  hash_finish(ctx, result, digest_size);
}


sha_1_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha_1_t>(BCRYPT_SHA1_ALGORITHM))
{}


sha_1_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void sha_1_t::hash_t::add (const void *data, size_t size) noexcept
{
  hash_add(ctx, data, size);
}


void sha_1_t::hash_t::finish (void *result) noexcept
{
  hash_finish(ctx, result, digest_size);
}


sha_256_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha_256_t>(BCRYPT_SHA256_ALGORITHM))
{}


sha_256_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void sha_256_t::hash_t::add (const void *data, size_t size) noexcept
{
  hash_add(ctx, data, size);
}


void sha_256_t::hash_t::finish (void *result) noexcept
{
  hash_finish(ctx, result, digest_size);
}


sha_384_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha_384_t>(BCRYPT_SHA384_ALGORITHM))
{}


sha_384_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void sha_384_t::hash_t::add (const void *data, size_t size) noexcept
{
  hash_add(ctx, data, size);
}


void sha_384_t::hash_t::finish (void *result) noexcept
{
  hash_finish(ctx, result, digest_size);
}


sha_512_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha_512_t>(BCRYPT_SHA512_ALGORITHM))
{}


sha_512_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void sha_512_t::hash_t::add (const void *data, size_t size) noexcept
{
  hash_add(ctx, data, size);
}


void sha_512_t::hash_t::finish (void *result) noexcept
{
  hash_finish(ctx, result, digest_size);
}


#endif // __sal_os_windows


}} // namespace crypto::__bits


__sal_end
