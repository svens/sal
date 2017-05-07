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
  #include <linux/if_alg.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

__sal_begin

namespace crypto { namespace __bits {


#if __sal_os_darwin // {{{1


md5_t::hash_t::hash_t () // {{{2
{
  CC_MD5_Init(this);
}


void md5_t::hash_t::update (const void *data, size_t size)
{
  CC_MD5_Update(this, data, size);
}


void md5_t::hash_t::finish (void *result)
{
  CC_MD5_Final(static_cast<uint8_t *>(result), this);
  CC_MD5_Init(this);
}


md5_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  CCHmacInit(&original, kCCHmacAlgMD5, key, size);
  current = original;
}


void md5_t::hmac_t::update (const void *data, size_t size)
{
  CCHmacUpdate(&current, data, size);
}


void md5_t::hmac_t::finish (void *result)
{
  CCHmacFinal(&current, result);
  current = original;
}


sha1_t::hash_t::hash_t () // {{{2
{
  CC_SHA1_Init(this);
}


void sha1_t::hash_t::update (const void *data, size_t size)
{
  CC_SHA1_Update(this, data, size);
}


void sha1_t::hash_t::finish (void *result)
{
  CC_SHA1_Final(static_cast<uint8_t *>(result), this);
  CC_SHA1_Init(this);
}


sha1_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  CCHmacInit(&original, kCCHmacAlgSHA1, key, size);
  current = original;
}


void sha1_t::hmac_t::update (const void *data, size_t size)
{
  CCHmacUpdate(&current, data, size);
}


void sha1_t::hmac_t::finish (void *result)
{
  CCHmacFinal(&current, result);
  current = original;
}


sha256_t::hash_t::hash_t () // {{{2
{
  CC_SHA256_Init(this);
}


void sha256_t::hash_t::update (const void *data, size_t size)
{
  CC_SHA256_Update(this, data, size);
}


void sha256_t::hash_t::finish (void *result)
{
  CC_SHA256_Final(static_cast<uint8_t *>(result), this);
  CC_SHA256_Init(this);
}


sha256_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  CCHmacInit(&original, kCCHmacAlgSHA256, key, size);
  current = original;
}


void sha256_t::hmac_t::update (const void *data, size_t size)
{
  CCHmacUpdate(&current, data, size);
}


void sha256_t::hmac_t::finish (void *result)
{
  CCHmacFinal(&current, result);
  current = original;
}


sha384_t::hash_t::hash_t () // {{{2
{
  CC_SHA384_Init(this);
}


void sha384_t::hash_t::update (const void *data, size_t size)
{
  CC_SHA384_Update(this, data, size);
}


void sha384_t::hash_t::finish (void *result)
{
  CC_SHA384_Final(static_cast<uint8_t *>(result), this);
  CC_SHA384_Init(this);
}


sha384_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  CCHmacInit(&original, kCCHmacAlgSHA384, key, size);
  current = original;
}


void sha384_t::hmac_t::update (const void *data, size_t size)
{
  CCHmacUpdate(&current, data, size);
}


void sha384_t::hmac_t::finish (void *result)
{
  CCHmacFinal(&current, result);
  current = original;
}


sha512_t::hash_t::hash_t () // {{{2
{
  CC_SHA512_Init(this);
}


void sha512_t::hash_t::update (const void *data, size_t size)
{
  CC_SHA512_Update(this, data, size);
}


void sha512_t::hash_t::finish (void *result)
{
  CC_SHA512_Final(static_cast<uint8_t *>(result), this);
  CC_SHA512_Init(this);
}


sha512_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  CCHmacInit(&original, kCCHmacAlgSHA512, key, size);
  current = original;
}


void sha512_t::hmac_t::update (const void *data, size_t size)
{
  CCHmacUpdate(&current, data, size);
}


void sha512_t::hmac_t::finish (void *result)
{
  CCHmacFinal(&current, result);
  current = original;
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


template <typename Algorithm,
  DWORD flags_if_hmac = BCRYPT_ALG_HANDLE_HMAC_FLAG
>
uintptr_t make_worker (LPCWSTR id,
  const void *key = nullptr,
  size_t size = 0U)
{
  static algorithm_driver_t driver(id, flags_if_hmac, Algorithm::digest_size);

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


} // namespace


basic_hash_t::basic_hash_t (basic_hash_t &&that) noexcept
  : handle(that.handle)
{
  that.handle = 0U;
}


basic_hash_t &basic_hash_t::operator= (basic_hash_t &&that) noexcept
{
  using std::swap;
  auto tmp{std::move(that)};
  swap(handle, tmp.handle);
  return *this;
}


basic_hash_t::~basic_hash_t () noexcept
{
  if (handle != 0U)
  {
    ::BCryptDestroyHash(reinterpret_cast<BCRYPT_HASH_HANDLE>(handle));
  }
}

void basic_hash_t::update (const void *data, size_t size)
{
  call(::BCryptHashData,
    reinterpret_cast<BCRYPT_HASH_HANDLE>(handle),
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(size),
    0
  );
}


void basic_hash_t::finish (void *result, size_t size)
{
  call(::BCryptFinishHash,
    reinterpret_cast<BCRYPT_HASH_HANDLE>(handle),
    static_cast<PUCHAR>(result),
    static_cast<ULONG>(size),
    0
  );
}


md5_t::hash_t::hash_t () // {{{2
  : basic_hash_t(make_worker<md5_t, 0>(BCRYPT_MD5_ALGORITHM))
{}


void md5_t::hash_t::update (const void *data, size_t size)
{
  basic_hash_t::update(data, size);
}


void md5_t::hash_t::finish (void *result)
{
  basic_hash_t::finish(result, digest_size);
}


md5_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : basic_hmac_t(make_worker<md5_t>(BCRYPT_MD5_ALGORITHM, key, size))
{}


void md5_t::hmac_t::update (const void *data, size_t size)
{
  basic_hmac_t::update(data, size);
}


void md5_t::hmac_t::finish (void *result)
{
  basic_hmac_t::finish(result, digest_size);
}


sha1_t::hash_t::hash_t () // {{{2
  : basic_hash_t(make_worker<sha1_t, 0>(BCRYPT_SHA1_ALGORITHM))
{}


void sha1_t::hash_t::update (const void *data, size_t size)
{
  basic_hash_t::update(data, size);
}


void sha1_t::hash_t::finish (void *result)
{
  basic_hash_t::finish(result, digest_size);
}


sha1_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : basic_hmac_t(make_worker<sha1_t>(BCRYPT_SHA1_ALGORITHM, key, size))
{}


void sha1_t::hmac_t::update (const void *data, size_t size)
{
  basic_hmac_t::update(data, size);
}


void sha1_t::hmac_t::finish (void *result)
{
  basic_hmac_t::finish(result, digest_size);
}


sha256_t::hash_t::hash_t () // {{{2
  : basic_hash_t(make_worker<sha256_t, 0>(BCRYPT_SHA256_ALGORITHM))
{}


void sha256_t::hash_t::update (const void *data, size_t size)
{
  basic_hash_t::update(data, size);
}


void sha256_t::hash_t::finish (void *result)
{
  basic_hash_t::finish(result, digest_size);
}


sha256_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : basic_hmac_t(make_worker<sha256_t>(BCRYPT_SHA256_ALGORITHM, key, size))
{}


void sha256_t::hmac_t::update (const void *data, size_t size)
{
  basic_hmac_t::update(data, size);
}


void sha256_t::hmac_t::finish (void *result)
{
  basic_hmac_t::finish(result, digest_size);
}


sha384_t::hash_t::hash_t () // {{{2
  : basic_hash_t(make_worker<sha384_t, 0>(BCRYPT_SHA384_ALGORITHM))
{}


void sha384_t::hash_t::update (const void *data, size_t size)
{
  basic_hash_t::update(data, size);
}


void sha384_t::hash_t::finish (void *result)
{
  basic_hash_t::finish(result, digest_size);
}


sha384_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : basic_hmac_t(make_worker<sha384_t>(BCRYPT_SHA384_ALGORITHM, key, size))
{}


void sha384_t::hmac_t::update (const void *data, size_t size)
{
  basic_hmac_t::update(data, size);
}


void sha384_t::hmac_t::finish (void *result)
{
  basic_hmac_t::finish(result, digest_size);
}


sha512_t::hash_t::hash_t () // {{{2
  : basic_hash_t(make_worker<sha512_t, 0>(BCRYPT_SHA512_ALGORITHM))
{}


void sha512_t::hash_t::update (const void *data, size_t size)
{
  basic_hash_t::update(data, size);
}


void sha512_t::hash_t::finish (void *result)
{
  basic_hash_t::finish(result, digest_size);
}


sha512_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : basic_hmac_t(make_worker<sha512_t>(BCRYPT_SHA512_ALGORITHM, key, size))
{}


void sha512_t::hmac_t::update (const void *data, size_t size)
{
  basic_hmac_t::update(data, size);
}


void sha512_t::hmac_t::finish (void *result)
{
  basic_hmac_t::finish(result, digest_size);
}


#endif // __sal_os_windows


}} // namespace crypto::__bits


__sal_end
