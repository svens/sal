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

#if __sal_os_linux
  #include <memory>
  #include <linux/if_alg.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif


__sal_begin

namespace crypto { namespace __bits {


#if __sal_os_darwin // {{{1


md5_t::hash_t::hash_t () // {{{2
{
  CC_MD5_Init(&ctx);
}


md5_t::hash_t::~hash_t () noexcept
{}


void md5_t::hash_t::add (const void *data, size_t size)
{
  CC_MD5_Update(&ctx, data, size);
}


void md5_t::hash_t::finish (void *result)
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


void sha_1_t::hash_t::add (const void *data, size_t size)
{
  CC_SHA1_Update(&ctx, data, size);
}


void sha_1_t::hash_t::finish (void *result)
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


void sha_256_t::hash_t::add (const void *data, size_t size)
{
  CC_SHA256_Update(&ctx, data, size);
}


void sha_256_t::hash_t::finish (void *result)
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


void sha_384_t::hash_t::add (const void *data, size_t size)
{
  CC_SHA384_Update(&ctx, data, size);
}


void sha_384_t::hash_t::finish (void *result)
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


void sha_512_t::hash_t::add (const void *data, size_t size)
{
  CC_SHA512_Update(&ctx, data, size);
}


void sha_512_t::hash_t::finish (void *result)
{
  CC_SHA512_Final(static_cast<uint8_t *>(result), &ctx);
  CC_SHA512_Init(&ctx);
}


#elif __sal_os_linux // {{{1


namespace {


struct algorithm_driver_t
{
  int handle = -1;

  template <size_t M, size_t N>
  algorithm_driver_t (const char (&type)[M], const char (&name)[N]);

  ~algorithm_driver_t () noexcept
  {
    if (handle != -1)
    {
      ::close(handle);
    }
  }

  int make_worker ();
};


template <size_t M, size_t N>
algorithm_driver_t::algorithm_driver_t (const char (&type)[M], const char (&name)[N])
  : handle(::socket(AF_ALG, SOCK_SEQPACKET, 0))
{
  if (handle == -1)
  {
    throw_system_error(
      std::error_code(errno, std::generic_category()),
      "af_alg.socket"
    );
  }

  sockaddr_alg alg{};
  alg.salg_family = AF_ALG;

  static_assert(sizeof(alg.salg_type) >= M, "too long algorithm type name");
  std::uninitialized_copy(type, type + M, reinterpret_cast<char *>(alg.salg_type));

  static_assert(sizeof(alg.salg_name) >= N, "too long algorithm name");
  std::uninitialized_copy(name, name + N, reinterpret_cast<char *>(alg.salg_name));

  if (::bind(handle, reinterpret_cast<sockaddr *>(&alg), sizeof(alg)) == -1)
  {
    throw_system_error(
      std::error_code(errno, std::generic_category()),
      "af_alg.bind"
    );
  }
}


int algorithm_driver_t::make_worker ()
{
  auto worker = ::accept(handle, nullptr, 0);
  if (worker != -1)
  {
    return worker;
  }

  throw_system_error(
    std::error_code(errno, std::generic_category()),
    "af_alg.accept"
  );
}


template <typename Algorithm, size_t N>
int make_hash (const char (&algorithm_name)[N])
{
  static algorithm_driver_t driver{"hash", algorithm_name};
  return driver.make_worker();
}


void hash_release (int handle) noexcept
{
  if (handle != -1)
  {
    (void)::close(handle);
  }
}


void hash_add (int handle, const void *data, size_t size) noexcept
{
  // MSG_MORE: more data to come, do not calculate hash yet
  if (::send(handle, data, size, MSG_MORE) != static_cast<ssize_t>(size))
  {
    throw_system_error(
      std::error_code(errno, std::generic_category()),
      "af_alg.send"
    );
  }
}


void hash_finish (int handle, void *result, size_t size) noexcept
{
  // final block, calculate hash
  if (::send(handle, "", 0, 0) == -1)
  {
    throw_system_error(
      std::error_code(errno, std::generic_category()),
      "af_alg.send"
    );
  }

  iovec iov{};
  iov.iov_base = result;
  iov.iov_len = size;

  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  if (::recvmsg(handle, &msg, 0) == -1)
  {
    throw_system_error(
      std::error_code(errno, std::generic_category()),
      "af_alg.recvmsg"
    );
  }
}


} // namespace


md5_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<md5_t>("md5"))
{}


md5_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void md5_t::hash_t::add (const void *data, size_t size)
{
  hash_add(ctx, data, size);
}


void md5_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha_1_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha_1_t>("sha1"))
{}


sha_1_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void sha_1_t::hash_t::add (const void *data, size_t size)
{
  hash_add(ctx, data, size);
}


void sha_1_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha_256_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha_256_t>("sha256"))
{}


sha_256_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void sha_256_t::hash_t::add (const void *data, size_t size)
{
  hash_add(ctx, data, size);
}


void sha_256_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha_384_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha_384_t>("sha384"))
{}


sha_384_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void sha_384_t::hash_t::add (const void *data, size_t size)
{
  hash_add(ctx, data, size);
}


void sha_384_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha_512_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha_512_t>("sha512"))
{}


sha_512_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void sha_512_t::hash_t::add (const void *data, size_t size)
{
  hash_add(ctx, data, size);
}


void sha_512_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
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


md5_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<md5_t>(BCRYPT_MD5_ALGORITHM))
{}


md5_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


void md5_t::hash_t::add (const void *data, size_t size)
{
  hash_add(ctx, data, size);
}


void md5_t::hash_t::finish (void *result)
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


void sha_1_t::hash_t::add (const void *data, size_t size)
{
  hash_add(ctx, data, size);
}


void sha_1_t::hash_t::finish (void *result)
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


void sha_256_t::hash_t::add (const void *data, size_t size)
{
  hash_add(ctx, data, size);
}


void sha_256_t::hash_t::finish (void *result)
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


void sha_384_t::hash_t::add (const void *data, size_t size)
{
  hash_add(ctx, data, size);
}


void sha_384_t::hash_t::finish (void *result)
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


void sha_512_t::hash_t::add (const void *data, size_t size)
{
  hash_add(ctx, data, size);
}


void sha_512_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


#endif // __sal_os_windows


}} // namespace crypto::__bits


__sal_end
