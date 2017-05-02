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

#include <sal/crypto/__bits/algorithm.hpp>
#include <sal/assert.hpp>
#include <sal/error.hpp>


__sal_begin

namespace crypto { namespace __bits {


#if __sal_os_windows


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


template <typename T>
uintptr_t hash_impl (LPCWSTR id)
{
  static algorithm_provider_t algorithm{id, 0, T::digest_size()};

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


} // namespace


basic_hash_t::~basic_hash_t () noexcept
{
  ::BCryptDestroyHash(reinterpret_cast<BCRYPT_HASH_HANDLE>(handle_));
}


void basic_hash_t::add (const void *data, size_t length)
{
  call(::BCryptHashData,
    reinterpret_cast<BCRYPT_HASH_HANDLE>(handle_),
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(length),
    0
  );
}


void basic_hash_t::finish (const void *data, size_t length)
{
  call(::BCryptFinishHash,
    reinterpret_cast<BCRYPT_HASH_HANDLE>(handle_),
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(length),
    0
  );
}


md2_t::md2_t ()
  : basic_hash_t{hash_impl<md2_t>(BCRYPT_MD2_ALGORITHM)}
{}


md4_t::md4_t ()
  : basic_hash_t{hash_impl<md4_t>(BCRYPT_MD4_ALGORITHM)}
{}


md5_t::md5_t ()
  : basic_hash_t{hash_impl<md5_t>(BCRYPT_MD5_ALGORITHM)}
{}


sha_1_t::sha_1_t ()
  : basic_hash_t{hash_impl<sha_1_t>(BCRYPT_SHA1_ALGORITHM)}
{}


sha_256_t::sha_256_t ()
  : basic_hash_t{hash_impl<sha_256_t>(BCRYPT_SHA256_ALGORITHM)}
{}


sha_384_t::sha_384_t ()
  : basic_hash_t{hash_impl<sha_384_t>(BCRYPT_SHA384_ALGORITHM)}
{}


sha_512_t::sha_512_t ()
  : basic_hash_t{hash_impl<sha_512_t>(BCRYPT_SHA512_ALGORITHM)}
{}


#endif


}} // namespace crypto::__bits


__sal_end
