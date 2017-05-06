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
#include <memory>

#if __sal_os_linux
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


md5_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{}


md5_t::hash_t &md5_t::hash_t::operator= (hash_t &&that) noexcept
{
  std::uninitialized_copy(&that.ctx, &that.ctx + 1, &ctx);
  return *this;
}


md5_t::hash_t::~hash_t () noexcept
{}


void md5_t::hash_t::update (const void *data, size_t size)
{
  CC_MD5_Update(&ctx, data, size);
}


void md5_t::hash_t::finish (void *result)
{
  CC_MD5_Final(static_cast<uint8_t *>(result), &ctx);
  CC_MD5_Init(&ctx);
}


md5_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  CCHmacInit(&ctx[0], kCCHmacAlgMD5, key, size);
  std::uninitialized_copy(&ctx[0], &ctx[1], &ctx[1]);
}


md5_t::hmac_t::hmac_t (hmac_t &&that) noexcept
  : ctx{that.ctx[0], that.ctx[1]}
{}


md5_t::hmac_t &md5_t::hmac_t::operator= (hmac_t &&that) noexcept
{
  std::uninitialized_copy(&that.ctx[0], &that.ctx[2], &ctx[0]);
  return *this;
}


md5_t::hmac_t::~hmac_t () noexcept
{}


void md5_t::hmac_t::update (const void *data, size_t size)
{
  CCHmacUpdate(&ctx[0], data, size);
}


void md5_t::hmac_t::finish (void *result)
{
  CCHmacFinal(&ctx[0], result);
  std::uninitialized_copy(&ctx[1], &ctx[2], &ctx[0]);
}


sha1_t::hash_t::hash_t () // {{{2
{
  CC_SHA1_Init(&ctx);
}


sha1_t::hash_t::~hash_t () noexcept
{}


sha1_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{}


sha1_t::hash_t &sha1_t::hash_t::operator= (hash_t &&that) noexcept
{
  std::uninitialized_copy(&that.ctx, &that.ctx + 1, &ctx);
  return *this;
}


void sha1_t::hash_t::update (const void *data, size_t size)
{
  CC_SHA1_Update(&ctx, data, size);
}


void sha1_t::hash_t::finish (void *result)
{
  CC_SHA1_Final(static_cast<uint8_t *>(result), &ctx);
  CC_SHA1_Init(&ctx);
}


sha1_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  CCHmacInit(&ctx[0], kCCHmacAlgSHA1, key, size);
  std::uninitialized_copy(&ctx[0], &ctx[1], &ctx[1]);
}


sha1_t::hmac_t::hmac_t (hmac_t &&that) noexcept
  : ctx{that.ctx[0], that.ctx[1]}
{}


sha1_t::hmac_t &sha1_t::hmac_t::operator= (hmac_t &&that) noexcept
{
  std::uninitialized_copy(&that.ctx[0], &that.ctx[2], &ctx[0]);
  return *this;
}


sha1_t::hmac_t::~hmac_t () noexcept
{}


void sha1_t::hmac_t::update (const void *data, size_t size)
{
  CCHmacUpdate(&ctx[0], data, size);
}


void sha1_t::hmac_t::finish (void *result)
{
  CCHmacFinal(&ctx[0], result);
  std::uninitialized_copy(&ctx[1], &ctx[2], &ctx[0]);
}


sha256_t::hash_t::hash_t () // {{{2
{
  CC_SHA256_Init(&ctx);
}


sha256_t::hash_t::~hash_t () noexcept
{}


sha256_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{}


sha256_t::hash_t &sha256_t::hash_t::operator= (hash_t &&that) noexcept
{
  std::uninitialized_copy(&that.ctx, &that.ctx + 1, &ctx);
  return *this;
}


void sha256_t::hash_t::update (const void *data, size_t size)
{
  CC_SHA256_Update(&ctx, data, size);
}


void sha256_t::hash_t::finish (void *result)
{
  CC_SHA256_Final(static_cast<uint8_t *>(result), &ctx);
  CC_SHA256_Init(&ctx);
}


sha256_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  CCHmacInit(&ctx[0], kCCHmacAlgSHA256, key, size);
  std::uninitialized_copy(&ctx[0], &ctx[1], &ctx[1]);
}


sha256_t::hmac_t::hmac_t (hmac_t &&that) noexcept
  : ctx{that.ctx[0], that.ctx[1]}
{}


sha256_t::hmac_t &sha256_t::hmac_t::operator= (hmac_t &&that) noexcept
{
  std::uninitialized_copy(&that.ctx[0], &that.ctx[2], &ctx[0]);
  return *this;
}


sha256_t::hmac_t::~hmac_t () noexcept
{}


void sha256_t::hmac_t::update (const void *data, size_t size)
{
  CCHmacUpdate(&ctx[0], data, size);
}


void sha256_t::hmac_t::finish (void *result)
{
  CCHmacFinal(&ctx[0], result);
  std::uninitialized_copy(&ctx[1], &ctx[2], &ctx[0]);
}


sha384_t::hash_t::hash_t () // {{{2
{
  CC_SHA384_Init(&ctx);
}


sha384_t::hash_t::~hash_t () noexcept
{}


sha384_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{}


sha384_t::hash_t &sha384_t::hash_t::operator= (hash_t &&that) noexcept
{
  std::uninitialized_copy(&that.ctx, &that.ctx + 1, &ctx);
  return *this;
}


void sha384_t::hash_t::update (const void *data, size_t size)
{
  CC_SHA384_Update(&ctx, data, size);
}


void sha384_t::hash_t::finish (void *result)
{
  CC_SHA384_Final(static_cast<uint8_t *>(result), &ctx);
  CC_SHA384_Init(&ctx);
}


sha384_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  CCHmacInit(&ctx[0], kCCHmacAlgSHA384, key, size);
  std::uninitialized_copy(&ctx[0], &ctx[1], &ctx[1]);
}


sha384_t::hmac_t::hmac_t (hmac_t &&that) noexcept
  : ctx{that.ctx[0], that.ctx[1]}
{}


sha384_t::hmac_t &sha384_t::hmac_t::operator= (hmac_t &&that) noexcept
{
  std::uninitialized_copy(&that.ctx[0], &that.ctx[2], &ctx[0]);
  return *this;
}


sha384_t::hmac_t::~hmac_t () noexcept
{}


void sha384_t::hmac_t::update (const void *data, size_t size)
{
  CCHmacUpdate(&ctx[0], data, size);
}


void sha384_t::hmac_t::finish (void *result)
{
  CCHmacFinal(&ctx[0], result);
  std::uninitialized_copy(&ctx[1], &ctx[2], &ctx[0]);
}


sha512_t::hash_t::hash_t () // {{{2
{
  CC_SHA512_Init(&ctx);
}


sha512_t::hash_t::~hash_t () noexcept
{}


sha512_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{}


sha512_t::hash_t &sha512_t::hash_t::operator= (hash_t &&that) noexcept
{
  std::uninitialized_copy(&that.ctx, &that.ctx + 1, &ctx);
  return *this;
}


void sha512_t::hash_t::update (const void *data, size_t size)
{
  CC_SHA512_Update(&ctx, data, size);
}


void sha512_t::hash_t::finish (void *result)
{
  CC_SHA512_Final(static_cast<uint8_t *>(result), &ctx);
  CC_SHA512_Init(&ctx);
}


sha512_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  CCHmacInit(&ctx[0], kCCHmacAlgSHA512, key, size);
  std::uninitialized_copy(&ctx[0], &ctx[1], &ctx[1]);
}


sha512_t::hmac_t::hmac_t (hmac_t &&that) noexcept
  : ctx{that.ctx[0], that.ctx[1]}
{}


sha512_t::hmac_t &sha512_t::hmac_t::operator= (hmac_t &&that) noexcept
{
  std::uninitialized_copy(&that.ctx[0], &that.ctx[2], &ctx[0]);
  return *this;
}


sha512_t::hmac_t::~hmac_t () noexcept
{}


void sha512_t::hmac_t::update (const void *data, size_t size)
{
  CCHmacUpdate(&ctx[0], data, size);
}


void sha512_t::hmac_t::finish (void *result)
{
  CCHmacFinal(&ctx[0], result);
  std::uninitialized_copy(&ctx[1], &ctx[2], &ctx[0]);
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


void hash_update (int handle, const void *data, size_t size) noexcept
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


md5_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = -1;
}


md5_t::hash_t &md5_t::hash_t::operator= (hash_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void md5_t::hash_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void md5_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha1_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha1_t>("sha1"))
{}


sha1_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


sha1_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = -1;
}


sha1_t::hash_t &sha1_t::hash_t::operator= (hash_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha1_t::hash_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha1_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha256_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha256_t>("sha256"))
{}


sha256_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


sha256_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = -1;
}


sha256_t::hash_t &sha256_t::hash_t::operator= (hash_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha256_t::hash_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha256_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha384_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha384_t>("sha384"))
{}


sha384_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


sha384_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = -1;
}


sha384_t::hash_t &sha384_t::hash_t::operator= (hash_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha384_t::hash_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha384_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha512_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha512_t>("sha512"))
{}


sha512_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


sha512_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = -1;
}


sha512_t::hash_t &sha512_t::hash_t::operator= (hash_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha512_t::hash_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha512_t::hash_t::finish (void *result)
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


struct algorithm_driver_t
{
  BCRYPT_ALG_HANDLE handle;

  algorithm_driver_t (LPCWSTR id, DWORD flags, size_t expected_hash_length);

  ~algorithm_driver_t () noexcept
  {
    ::BCryptCloseAlgorithmProvider(handle, 0);
  }
};


algorithm_driver_t::algorithm_driver_t (LPCWSTR id,
  DWORD flags,
  size_t expected_hash_length)
{
  flags |= BCRYPT_HASH_REUSABLE_FLAG;
  call(::BCryptOpenAlgorithmProvider, &handle, id, nullptr, flags);

  DWORD hash_length, cbData;
  call(::BCryptGetProperty,
    handle,
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
  static algorithm_driver_t driver(id, 0, Algorithm::digest_size);

  BCRYPT_HASH_HANDLE handle;
  call(::BCryptCreateHash,
    driver.handle,
    &handle,
    nullptr, 0,
    nullptr, 0,
    BCRYPT_HASH_REUSABLE_FLAG
  );
  return reinterpret_cast<uintptr_t>(handle);
}


template <typename Algorithm>
uintptr_t make_hmac (LPCWSTR id, const void *key, size_t size)
{
  static algorithm_driver_t driver(id,
    BCRYPT_ALG_HANDLE_HMAC_FLAG,
    Algorithm::digest_size
  );

  BCRYPT_HASH_HANDLE handle;
  call(::BCryptCreateHash,
    driver.handle,
    &handle,
    nullptr, 0,
    static_cast<PUCHAR>(const_cast<void *>(key)),
    static_cast<ULONG>(size),
    BCRYPT_HASH_REUSABLE_FLAG
  );
  return reinterpret_cast<uintptr_t>(handle);
}


void hash_release (uintptr_t handle) noexcept
{
  if (handle != 0U)
  {
    ::BCryptDestroyHash(reinterpret_cast<BCRYPT_HASH_HANDLE>(handle));
  }
}


void hash_update (uintptr_t handle, const void *data, size_t size) noexcept
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


md5_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = 0;
}


md5_t::hash_t &md5_t::hash_t::operator= (hash_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void md5_t::hash_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void md5_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


md5_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : ctx(make_hmac<md5_t>(BCRYPT_MD5_ALGORITHM, key, size))
{}


md5_t::hmac_t::~hmac_t () noexcept
{
  hash_release(ctx);
}


md5_t::hmac_t::hmac_t (hmac_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = 0;
}


md5_t::hmac_t &md5_t::hmac_t::operator= (hmac_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void md5_t::hmac_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void md5_t::hmac_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha1_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha1_t>(BCRYPT_SHA1_ALGORITHM))
{}


sha1_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


sha1_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = 0;
}


sha1_t::hash_t &sha1_t::hash_t::operator= (hash_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha1_t::hash_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha1_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha1_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : ctx(make_hmac<sha1_t>(BCRYPT_SHA1_ALGORITHM, key, size))
{}


sha1_t::hmac_t::~hmac_t () noexcept
{
  hash_release(ctx);
}


sha1_t::hmac_t::hmac_t (hmac_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = 0;
}


sha1_t::hmac_t &sha1_t::hmac_t::operator= (hmac_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha1_t::hmac_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha1_t::hmac_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha256_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha256_t>(BCRYPT_SHA256_ALGORITHM))
{}


sha256_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


sha256_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = 0;
}


sha256_t::hash_t &sha256_t::hash_t::operator= (hash_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha256_t::hash_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha256_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha256_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : ctx(make_hmac<sha256_t>(BCRYPT_SHA256_ALGORITHM, key, size))
{}


sha256_t::hmac_t::~hmac_t () noexcept
{
  hash_release(ctx);
}


sha256_t::hmac_t::hmac_t (hmac_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = 0;
}


sha256_t::hmac_t &sha256_t::hmac_t::operator= (hmac_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha256_t::hmac_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha256_t::hmac_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha384_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha384_t>(BCRYPT_SHA384_ALGORITHM))
{}


sha384_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


sha384_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = 0;
}


sha384_t::hash_t &sha384_t::hash_t::operator= (hash_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha384_t::hash_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha384_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha384_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : ctx(make_hmac<sha384_t>(BCRYPT_SHA384_ALGORITHM, key, size))
{}


sha384_t::hmac_t::~hmac_t () noexcept
{
  hash_release(ctx);
}


sha384_t::hmac_t::hmac_t (hmac_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = 0;
}


sha384_t::hmac_t &sha384_t::hmac_t::operator= (hmac_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha384_t::hmac_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha384_t::hmac_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha512_t::hash_t::hash_t () // {{{2
  : ctx(make_hash<sha512_t>(BCRYPT_SHA512_ALGORITHM))
{}


sha512_t::hash_t::~hash_t () noexcept
{
  hash_release(ctx);
}


sha512_t::hash_t::hash_t (hash_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = 0;
}


sha512_t::hash_t &sha512_t::hash_t::operator= (hash_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha512_t::hash_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha512_t::hash_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


sha512_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : ctx(make_hmac<sha512_t>(BCRYPT_SHA512_ALGORITHM, key, size))
{}


sha512_t::hmac_t::~hmac_t () noexcept
{
  hash_release(ctx);
}


sha512_t::hmac_t::hmac_t (hmac_t &&that) noexcept
  : ctx(that.ctx)
{
  that.ctx = 0;
}


sha512_t::hmac_t &sha512_t::hmac_t::operator= (hmac_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(ctx, tmp.ctx);
  return *this;
}


void sha512_t::hmac_t::update (const void *data, size_t size)
{
  hash_update(ctx, data, size);
}


void sha512_t::hmac_t::finish (void *result)
{
  hash_finish(ctx, result, digest_size);
}


#endif // __sal_os_windows


}} // namespace crypto::__bits


__sal_end
