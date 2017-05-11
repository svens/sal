#if defined(_WIN32) || defined(_WIN64)
  #define WIN32_NO_STATUS
  #include <windows.h>
  #include <bcrypt.h>
  #undef WIN32_NO_STATUS
  #include <winternl.h>
  #include <ntstatus.h>
  #pragma comment(lib, "ncrypt")
  #pragma comment(lib, "ntdll")
  #include <mutex>
#endif

#include <sal/crypto/__bits/hash_algorithm.hpp>
#include <sal/assert.hpp>
#include <sal/error.hpp>


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


void md5_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  CC_MD5(data, size, static_cast<uint8_t *>(result));
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


void md5_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  CCHmac(kCCHmacAlgMD5, key, key_size, data, data_size, result);
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


void sha1_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  CC_SHA1(data, size, static_cast<uint8_t *>(result));
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


void sha1_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  CCHmac(kCCHmacAlgSHA1, key, key_size, data, data_size, result);
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


void sha256_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  CC_SHA256(data, size, static_cast<uint8_t *>(result));
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


void sha256_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  CCHmac(kCCHmacAlgSHA256, key, key_size, data, data_size, result);
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


void sha384_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  CC_SHA384(data, size, static_cast<uint8_t *>(result));
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


void sha384_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  CCHmac(kCCHmacAlgSHA384, key, key_size, data, data_size, result);
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


void sha512_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  CC_SHA512(data, size, static_cast<uint8_t *>(result));
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


void sha512_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  CCHmac(kCCHmacAlgSHA512, key, key_size, data, data_size, result);
}


#elif __sal_os_linux // {{{1


namespace {

inline void fix_key (const void *&key, size_t &size) noexcept
{
  if (!key || !size)
  {
    key = "";
    size = 0U;
  }
}

} // namespace


basic_hmac_t::basic_hmac_t () noexcept
  : context{std::make_unique<HMAC_CTX>()}
{
  HMAC_CTX_init(context.get());
}


basic_hmac_t::~basic_hmac_t () noexcept
{
  if (context)
  {
    HMAC_CTX_cleanup(context.get());
  }
}


md5_t::hash_t::hash_t () // {{{2
{
  MD5_Init(this);
}


void md5_t::hash_t::update (const void *data, size_t size)
{
  MD5_Update(this, data, size);
}


void md5_t::hash_t::finish (void *result)
{
  MD5_Final(static_cast<uint8_t *>(result), this);
  MD5_Init(this);
}


void md5_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  MD5(static_cast<const uint8_t *>(data), size,
    static_cast<uint8_t *>(result)
  );
}


md5_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  fix_key(key, size);
  HMAC_Init_ex(context.get(), key, size, EVP_md5(), nullptr);
}


void md5_t::hmac_t::update (const void *data, size_t size)
{
  HMAC_Update(context.get(), static_cast<const uint8_t *>(data), size);
}


void md5_t::hmac_t::finish (void *result)
{
  unsigned size = digest_size;
  HMAC_Final(context.get(), static_cast<uint8_t *>(result), &size);
  HMAC_Init_ex(context.get(), nullptr, 0, nullptr, nullptr);
}


void md5_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  fix_key(key, key_size);
  HMAC(EVP_md5(),
    key, key_size,
    static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(result), nullptr
  );
}


sha1_t::hash_t::hash_t () // {{{2
{
  SHA1_Init(this);
}


void sha1_t::hash_t::update (const void *data, size_t size)
{
  SHA1_Update(this, data, size);
}


void sha1_t::hash_t::finish (void *result)
{
  SHA1_Final(static_cast<uint8_t *>(result), this);
  SHA1_Init(this);
}


void sha1_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  SHA1(static_cast<const uint8_t *>(data), size,
    static_cast<uint8_t *>(result)
  );
}


sha1_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  fix_key(key, size);
  HMAC_Init_ex(context.get(), key, size, EVP_sha1(), nullptr);
}


void sha1_t::hmac_t::update (const void *data, size_t size)
{
  HMAC_Update(context.get(), static_cast<const uint8_t *>(data), size);
}


void sha1_t::hmac_t::finish (void *result)
{
  unsigned size;
  HMAC_Final(context.get(), static_cast<uint8_t *>(result), &size);
  HMAC_Init_ex(context.get(), nullptr, 0, nullptr, nullptr);
}


void sha1_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  fix_key(key, key_size);
  HMAC(EVP_sha1(),
    key, key_size,
    static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(result), nullptr
  );
}


sha256_t::hash_t::hash_t () // {{{2
{
  SHA256_Init(this);
}


void sha256_t::hash_t::update (const void *data, size_t size)
{
  SHA256_Update(this, data, size);
}


void sha256_t::hash_t::finish (void *result)
{
  SHA256_Final(static_cast<uint8_t *>(result), this);
  SHA256_Init(this);
}


void sha256_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  SHA256(static_cast<const uint8_t *>(data), size,
    static_cast<uint8_t *>(result)
  );
}


sha256_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  fix_key(key, size);
  HMAC_Init_ex(context.get(), key, size, EVP_sha256(), nullptr);
}


void sha256_t::hmac_t::update (const void *data, size_t size)
{
  HMAC_Update(context.get(), static_cast<const uint8_t *>(data), size);
}


void sha256_t::hmac_t::finish (void *result)
{
  unsigned size;
  HMAC_Final(context.get(), static_cast<uint8_t *>(result), &size);
  HMAC_Init_ex(context.get(), nullptr, 0, nullptr, nullptr);
}


void sha256_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  fix_key(key, key_size);
  HMAC(EVP_sha256(),
    key, key_size,
    static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(result), nullptr
  );
}


sha384_t::hash_t::hash_t () // {{{2
{
  SHA384_Init(this);
}


void sha384_t::hash_t::update (const void *data, size_t size)
{
  SHA384_Update(this, data, size);
}


void sha384_t::hash_t::finish (void *result)
{
  SHA384_Final(static_cast<uint8_t *>(result), this);
  SHA384_Init(this);
}


void sha384_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  SHA384(static_cast<const uint8_t *>(data), size,
    static_cast<uint8_t *>(result)
  );
}


sha384_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  fix_key(key, size);
  HMAC_Init_ex(context.get(), key, size, EVP_sha384(), nullptr);
}


void sha384_t::hmac_t::update (const void *data, size_t size)
{
  HMAC_Update(context.get(), static_cast<const uint8_t *>(data), size);
}


void sha384_t::hmac_t::finish (void *result)
{
  unsigned size;
  HMAC_Final(context.get(), static_cast<uint8_t *>(result), &size);
  HMAC_Init_ex(context.get(), nullptr, 0, nullptr, nullptr);
}


void sha384_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  fix_key(key, key_size);
  HMAC(EVP_sha384(),
    key, key_size,
    static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(result), nullptr
  );
}


sha512_t::hash_t::hash_t () // {{{2
{
  SHA512_Init(this);
}


void sha512_t::hash_t::update (const void *data, size_t size)
{
  SHA512_Update(this, data, size);
}


void sha512_t::hash_t::finish (void *result)
{
  SHA512_Final(static_cast<uint8_t *>(result), this);
  SHA512_Init(this);
}


void sha512_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  SHA512(static_cast<const uint8_t *>(data), size,
    static_cast<uint8_t *>(result)
  );
}


sha512_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
{
  fix_key(key, size);
  HMAC_Init_ex(context.get(), key, size, EVP_sha512(), nullptr);
}


void sha512_t::hmac_t::update (const void *data, size_t size)
{
  HMAC_Update(context.get(), static_cast<const uint8_t *>(data), size);
}


void sha512_t::hmac_t::finish (void *result)
{
  unsigned size;
  HMAC_Final(context.get(), static_cast<uint8_t *>(result), &size);
  HMAC_Init_ex(context.get(), nullptr, 0, nullptr, nullptr);
}


void sha512_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  fix_key(key, key_size);
  HMAC(EVP_sha512(),
    key, key_size,
    static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(result), nullptr
  );
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


template <typename Algorithm> constexpr LPCWSTR provider_name_v = nullptr;
template <> constexpr LPCWSTR provider_name_v<md5_t> = BCRYPT_MD5_ALGORITHM;
template <> constexpr LPCWSTR provider_name_v<sha1_t> = BCRYPT_SHA1_ALGORITHM;
template <> constexpr LPCWSTR provider_name_v<sha256_t> = BCRYPT_SHA256_ALGORITHM;
template <> constexpr LPCWSTR provider_name_v<sha384_t> = BCRYPT_SHA384_ALGORITHM;
template <> constexpr LPCWSTR provider_name_v<sha512_t> = BCRYPT_SHA512_ALGORITHM;

template <bool IsHMAC> constexpr DWORD provider_flags_v = 0U;
template <> constexpr DWORD provider_flags_v<true> = BCRYPT_ALG_HANDLE_HMAC_FLAG;


struct algorithm_provider_t
{
  BCRYPT_ALG_HANDLE handle;


  algorithm_provider_t (LPCWSTR id, DWORD flags, size_t expected_hash_length)
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


  ~algorithm_provider_t () noexcept
  {
    ::BCryptCloseAlgorithmProvider(handle, 0);
  }
};


template <typename Algorithm, bool IsHMAC>
BCRYPT_ALG_HANDLE provider ()
{
  static algorithm_provider_t provider_
  {
    provider_name_v<Algorithm>,
    provider_flags_v<IsHMAC>,
    Algorithm::digest_size
  };
  return provider_.handle;
}


template <typename Algorithm, bool IsHMAC>
uintptr_t worker (const void *key = nullptr, size_t size = 0U)
{
  BCRYPT_HASH_HANDLE handle;
  call(::BCryptCreateHash,
    provider<Algorithm, IsHMAC>(),
    &handle,
    nullptr, 0,
    static_cast<PUCHAR>(const_cast<void *>(key)),
    static_cast<ULONG>(size),
    BCRYPT_HASH_REUSABLE_FLAG
  );
  return reinterpret_cast<uintptr_t>(handle);
}


template <typename Algorithm>
void hash (const void *data, size_t size, void *result)
{
  call(::BCryptHash,
    provider<Algorithm, false>(),
    nullptr, 0,
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(size),
    static_cast<PUCHAR>(result),
    static_cast<ULONG>(Algorithm::digest_size)
  );
}


template <typename Algorithm>
void hmac (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  call(::BCryptHash,
    provider<Algorithm, true>(),
    static_cast<PUCHAR>(const_cast<void *>(key)),
    static_cast<ULONG>(key_size),
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(data_size),
    static_cast<PUCHAR>(result),
    static_cast<ULONG>(Algorithm::digest_size)
  );
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
  : basic_hash_t(worker<md5_t, false>())
{}


void md5_t::hash_t::update (const void *data, size_t size)
{
  basic_hash_t::update(data, size);
}


void md5_t::hash_t::finish (void *result)
{
  basic_hash_t::finish(result, digest_size);
}


void md5_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  hash<md5_t>(data, size, result);
}


md5_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : basic_hmac_t(worker<md5_t, true>(key, size))
{}


void md5_t::hmac_t::update (const void *data, size_t size)
{
  basic_hmac_t::update(data, size);
}


void md5_t::hmac_t::finish (void *result)
{
  basic_hmac_t::finish(result, digest_size);
}


void md5_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  hmac<md5_t>(key, key_size, data, data_size, result);
}


sha1_t::hash_t::hash_t () // {{{2
  : basic_hash_t(worker<sha1_t, false>())
{}


void sha1_t::hash_t::update (const void *data, size_t size)
{
  basic_hash_t::update(data, size);
}


void sha1_t::hash_t::finish (void *result)
{
  basic_hash_t::finish(result, digest_size);
}


void sha1_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  hash<sha1_t>(data, size, result);
}


sha1_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : basic_hmac_t(worker<sha1_t, true>(key, size))
{}


void sha1_t::hmac_t::update (const void *data, size_t size)
{
  basic_hmac_t::update(data, size);
}


void sha1_t::hmac_t::finish (void *result)
{
  basic_hmac_t::finish(result, digest_size);
}


void sha1_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  hmac<sha1_t>(key, key_size, data, data_size, result);
}


sha256_t::hash_t::hash_t () // {{{2
  : basic_hash_t(worker<sha256_t, false>())
{}


void sha256_t::hash_t::update (const void *data, size_t size)
{
  basic_hash_t::update(data, size);
}


void sha256_t::hash_t::finish (void *result)
{
  basic_hash_t::finish(result, digest_size);
}


void sha256_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  hash<sha256_t>(data, size, result);
}


sha256_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : basic_hmac_t(worker<sha256_t, true>(key, size))
{}


void sha256_t::hmac_t::update (const void *data, size_t size)
{
  basic_hmac_t::update(data, size);
}


void sha256_t::hmac_t::finish (void *result)
{
  basic_hmac_t::finish(result, digest_size);
}


void sha256_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  hmac<sha256_t>(key, key_size, data, data_size, result);
}


sha384_t::hash_t::hash_t () // {{{2
  : basic_hash_t(worker<sha384_t, false>())
{}


void sha384_t::hash_t::update (const void *data, size_t size)
{
  basic_hash_t::update(data, size);
}


void sha384_t::hash_t::finish (void *result)
{
  basic_hash_t::finish(result, digest_size);
}


void sha384_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  hash<sha384_t>(data, size, result);
}


sha384_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : basic_hmac_t(worker<sha384_t, true>(key, size))
{}


void sha384_t::hmac_t::update (const void *data, size_t size)
{
  basic_hmac_t::update(data, size);
}


void sha384_t::hmac_t::finish (void *result)
{
  basic_hmac_t::finish(result, digest_size);
}


void sha384_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  hmac<sha384_t>(key, key_size, data, data_size, result);
}


sha512_t::hash_t::hash_t () // {{{2
  : basic_hash_t(worker<sha512_t, false>())
{}


void sha512_t::hash_t::update (const void *data, size_t size)
{
  basic_hash_t::update(data, size);
}


void sha512_t::hash_t::finish (void *result)
{
  basic_hash_t::finish(result, digest_size);
}


void sha512_t::hash_t::one_shot (const void *data, size_t size, void *result)
{
  hash<sha512_t>(data, size, result);
}


sha512_t::hmac_t::hmac_t (const void *key, size_t size) // {{{2
  : basic_hmac_t(worker<sha512_t, true>(key, size))
{}


void sha512_t::hmac_t::update (const void *data, size_t size)
{
  basic_hmac_t::update(data, size);
}


void sha512_t::hmac_t::finish (void *result)
{
  basic_hmac_t::finish(result, digest_size);
}


void sha512_t::hmac_t::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  hmac<sha512_t>(key, key_size, data, data_size, result);
}


#endif // __sal_os_windows


}} // namespace crypto::__bits


__sal_end
