#include <sal/__bits/platform_sdk.hpp>
#include <sal/crypto/hash.hpp>
#include <sal/assert.hpp>


__sal_begin


namespace crypto {


#if __sal_os_darwin

// md5 {{{1

template <>
hash_t<md5>::hash_t ()
{
  sal_verify(::CC_MD5_Init(&ctx_));
}

template <>
void hash_t<md5>::update (const void *data, size_t size)
{
  sal_verify(::CC_MD5_Update(&ctx_, data, size));
}

template <>
void hash_t<md5>::finish (void *digest, size_t)
{
  sal_verify(::CC_MD5_Final(static_cast<uint8_t *>(digest), &ctx_));
  sal_verify(::CC_MD5_Init(&ctx_));
}

template <>
void hash_t<md5>::one_shot (const void *data, size_t data_size,
  void *digest, size_t)
{
  ::CC_MD5(data, data_size, static_cast<uint8_t *>(digest));
}

// sha1 {{{1

template <>
hash_t<sha1>::hash_t ()
{
  sal_verify(::CC_SHA1_Init(&ctx_));
}

template <>
void hash_t<sha1>::update (const void *data, size_t size)
{
  sal_verify(::CC_SHA1_Update(&ctx_, data, size));
}

template <>
void hash_t<sha1>::finish (void *digest, size_t)
{
  sal_verify(::CC_SHA1_Final(static_cast<uint8_t *>(digest), &ctx_));
  sal_verify(::CC_SHA1_Init(&ctx_));
}

template <>
void hash_t<sha1>::one_shot (const void *data, size_t data_size,
  void *digest, size_t)
{
  ::CC_SHA1(data, data_size, static_cast<uint8_t *>(digest));
}

// sha256 {{{1

template <>
hash_t<sha256>::hash_t ()
{
  sal_verify(::CC_SHA256_Init(&ctx_));
}

template <>
void hash_t<sha256>::update (const void *data, size_t size)
{
  sal_verify(::CC_SHA256_Update(&ctx_, data, size));
}

template <>
void hash_t<sha256>::finish (void *digest, size_t)
{
  sal_verify(::CC_SHA256_Final(static_cast<uint8_t *>(digest), &ctx_));
  sal_verify(::CC_SHA256_Init(&ctx_));
}

template <>
void hash_t<sha256>::one_shot (const void *data, size_t data_size,
  void *digest, size_t)
{
  ::CC_SHA256(data, data_size, static_cast<uint8_t *>(digest));
}

// sha384 {{{1

template <>
hash_t<sha384>::hash_t ()
{
  sal_verify(::CC_SHA384_Init(&ctx_));
}

template <>
void hash_t<sha384>::update (const void *data, size_t size)
{
  sal_verify(::CC_SHA384_Update(&ctx_, data, size));
}

template <>
void hash_t<sha384>::finish (void *digest, size_t)
{
  sal_verify(::CC_SHA384_Final(static_cast<uint8_t *>(digest), &ctx_));
  sal_verify(::CC_SHA384_Init(&ctx_));
}

template <>
void hash_t<sha384>::one_shot (const void *data, size_t data_size,
  void *digest, size_t)
{
  ::CC_SHA384(data, data_size, static_cast<uint8_t *>(digest));
}

// sha512 {{{1

template <>
hash_t<sha512>::hash_t ()
{
  sal_verify(::CC_SHA512_Init(&ctx_));
}

template <>
void hash_t<sha512>::update (const void *data, size_t size)
{
  sal_verify(::CC_SHA512_Update(&ctx_, data, size));
}

template <>
void hash_t<sha512>::finish (void *digest, size_t)
{
  sal_verify(::CC_SHA512_Final(static_cast<uint8_t *>(digest), &ctx_));
  sal_verify(::CC_SHA512_Init(&ctx_));
}

template <>
void hash_t<sha512>::one_shot (const void *data, size_t data_size,
  void *digest, size_t)
{
  ::CC_SHA512(data, data_size, static_cast<uint8_t *>(digest));
}

// }}}

#elif __sal_os_linux

// md5 {{{1

template <>
hash_t<md5>::hash_t ()
{
  sal_verify(::MD5_Init(&ctx_));
}

template <>
void hash_t<md5>::update (const void *data, size_t size)
{
  sal_verify(::MD5_Update(&ctx_, data, size));
}

template <>
void hash_t<md5>::finish (void *digest, size_t)
{
  sal_verify(::MD5_Final(static_cast<uint8_t *>(digest), &ctx_));
  sal_verify(::MD5_Init(&ctx_));
}

template <>
void hash_t<md5>::one_shot (const void *data, size_t data_size,
  void *digest, size_t)
{
  ::MD5(static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(digest)
  );
}

// sha1 {{{1

template <>
hash_t<sha1>::hash_t ()
{
  sal_verify(::SHA1_Init(&ctx_));
}

template <>
void hash_t<sha1>::update (const void *data, size_t size)
{
  sal_verify(::SHA1_Update(&ctx_, data, size));
}

template <>
void hash_t<sha1>::finish (void *digest, size_t)
{
  sal_verify(::SHA1_Final(static_cast<uint8_t *>(digest), &ctx_));
  sal_verify(::SHA1_Init(&ctx_));
}

template <>
void hash_t<sha1>::one_shot (const void *data, size_t data_size,
  void *digest, size_t)
{
  ::SHA1(static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(digest)
  );
}

// sha256 {{{1

template <>
hash_t<sha256>::hash_t ()
{
  sal_verify(::SHA256_Init(&ctx_));
}

template <>
void hash_t<sha256>::update (const void *data, size_t size)
{
  sal_verify(::SHA256_Update(&ctx_, data, size));
}

template <>
void hash_t<sha256>::finish (void *digest, size_t)
{
  sal_verify(::SHA256_Final(static_cast<uint8_t *>(digest), &ctx_));
  sal_verify(::SHA256_Init(&ctx_));
}

template <>
void hash_t<sha256>::one_shot (const void *data, size_t data_size,
  void *digest, size_t)
{
  ::SHA256(static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(digest)
  );
}

// sha384 {{{1

template <>
hash_t<sha384>::hash_t ()
{
  sal_verify(::SHA384_Init(&ctx_));
}

template <>
void hash_t<sha384>::update (const void *data, size_t size)
{
  sal_verify(::SHA384_Update(&ctx_, data, size));
}

template <>
void hash_t<sha384>::finish (void *digest, size_t)
{
  sal_verify(::SHA384_Final(static_cast<uint8_t *>(digest), &ctx_));
  sal_verify(::SHA384_Init(&ctx_));
}

template <>
void hash_t<sha384>::one_shot (const void *data, size_t data_size,
  void *digest, size_t)
{
  ::SHA384(static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(digest)
  );
}

// sha512 {{{1

template <>
hash_t<sha512>::hash_t ()
{
  sal_verify(::SHA512_Init(&ctx_));
}

template <>
void hash_t<sha512>::update (const void *data, size_t size)
{
  sal_verify(::SHA512_Update(&ctx_, data, size));
}

template <>
void hash_t<sha512>::finish (void *digest, size_t)
{
  sal_verify(::SHA512_Final(static_cast<uint8_t *>(digest), &ctx_));
  sal_verify(::SHA512_Init(&ctx_));
}

template <>
void hash_t<sha512>::one_shot (const void *data, size_t data_size,
  void *digest, size_t)
{
  ::SHA512(static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(digest)
  );
}

// }}}

#elif __sal_os_windows

namespace __bits { // {{{1


namespace {

inline void check_call (NTSTATUS status, const char *func)
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
#define call(func,...) check_call(func(__VA_ARGS__), #func)


template <typename Algorithm> constexpr LPCWSTR algorithm_id_v = {};
template <> constexpr LPCWSTR algorithm_id_v<md5> = BCRYPT_MD5_ALGORITHM;
template <> constexpr LPCWSTR algorithm_id_v<sha1> = BCRYPT_SHA1_ALGORITHM;
template <> constexpr LPCWSTR algorithm_id_v<sha256> = BCRYPT_SHA256_ALGORITHM;
template <> constexpr LPCWSTR algorithm_id_v<sha384> = BCRYPT_SHA384_ALGORITHM;
template <> constexpr LPCWSTR algorithm_id_v<sha512> = BCRYPT_SHA512_ALGORITHM;

template <bool IsHMAC> constexpr DWORD algorithm_flags_v = 0U;
template <> constexpr DWORD algorithm_flags_v<true> = BCRYPT_ALG_HANDLE_HMAC_FLAG;


struct context_factory_t
{
  BCRYPT_ALG_HANDLE handle;

  context_factory_t (LPCWSTR id, DWORD flags, size_t expected_hash_length)
  {
    flags |= BCRYPT_HASH_REUSABLE_FLAG;
    call(::BCryptOpenAlgorithmProvider, &handle, id, nullptr, flags);

    DWORD hash_length, copied;
    call(::BCryptGetProperty,
      handle,
      BCRYPT_HASH_LENGTH,
      reinterpret_cast<PBYTE>(&hash_length),
      sizeof(hash_length),
      &copied,
      0
    );

    sal_assert(hash_length == expected_hash_length);
    (void)expected_hash_length;
  }

  ~context_factory_t () noexcept
  {
    ::BCryptCloseAlgorithmProvider(handle, 0);
  }
};


} // namespace


context_t::context_t (const context_t &that)
  : handle{}
{
  call(::BCryptDuplicateHash,
    that.handle,
    &handle,
    nullptr,
    0,
    0
  );
}


context_t::~context_t () noexcept
{
  if (handle)
  {
    call(::BCryptDestroyHash, handle);
  }
}


template <typename Algorithm, bool IsHMAC>
BCRYPT_ALG_HANDLE context_t::factory ()
{
  static context_factory_t factory_
  {
    algorithm_id_v<Algorithm>,
    algorithm_flags_v<IsHMAC>,
    digest_size_v<Algorithm>
  };
  return factory_.handle;
}

template BCRYPT_ALG_HANDLE context_t::factory<md5, false> ();
template BCRYPT_ALG_HANDLE context_t::factory<md5, true> ();
template BCRYPT_ALG_HANDLE context_t::factory<sha1, false> ();
template BCRYPT_ALG_HANDLE context_t::factory<sha1, true> ();
template BCRYPT_ALG_HANDLE context_t::factory<sha256, false> ();
template BCRYPT_ALG_HANDLE context_t::factory<sha256, true> ();
template BCRYPT_ALG_HANDLE context_t::factory<sha384, false> ();
template BCRYPT_ALG_HANDLE context_t::factory<sha384, true> ();
template BCRYPT_ALG_HANDLE context_t::factory<sha512, false> ();
template BCRYPT_ALG_HANDLE context_t::factory<sha512, true> ();


template <typename Algorithm, bool IsHMAC>
BCRYPT_HASH_HANDLE context_t::make (const void *key, size_t size)
{
  BCRYPT_HASH_HANDLE handle;
  call(::BCryptCreateHash, factory<Algorithm, IsHMAC>(),
    &handle,
    nullptr, 0,
    static_cast<PUCHAR>(const_cast<void *>(key)),
    static_cast<ULONG>(size),
    BCRYPT_HASH_REUSABLE_FLAG
  );
  return handle;
}

template BCRYPT_HASH_HANDLE context_t::make<md5, false> (const void *, size_t);
template BCRYPT_HASH_HANDLE context_t::make<md5, true> (const void *, size_t);
template BCRYPT_HASH_HANDLE context_t::make<sha1, false> (const void *, size_t);
template BCRYPT_HASH_HANDLE context_t::make<sha1, true> (const void *, size_t);
template BCRYPT_HASH_HANDLE context_t::make<sha256, false> (const void *, size_t);
template BCRYPT_HASH_HANDLE context_t::make<sha256, true> (const void *, size_t);
template BCRYPT_HASH_HANDLE context_t::make<sha384, false> (const void *, size_t);
template BCRYPT_HASH_HANDLE context_t::make<sha384, true> (const void *, size_t);
template BCRYPT_HASH_HANDLE context_t::make<sha512, false> (const void *, size_t);
template BCRYPT_HASH_HANDLE context_t::make<sha512, true> (const void *, size_t);


void context_t::update (const void *data, size_t size)
{
  call(::BCryptHashData, handle,
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(size),
    0
  );
}


void context_t::finish (void *digest, size_t size)
{
  call(::BCryptFinishHash, handle,
    static_cast<PUCHAR>(digest),
    static_cast<ULONG>(size),
    0
  );
}


void context_t::hash (BCRYPT_ALG_HANDLE algorithm,
  const void *data, size_t data_size,
  void *digest, size_t digest_size)
{
  call(::BCryptHash, algorithm,
    nullptr, 0,
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(data_size),
    static_cast<PUCHAR>(digest),
    static_cast<ULONG>(digest_size)
  );
}


void context_t::hmac (BCRYPT_ALG_HANDLE algorithm,
  const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *digest, size_t digest_size)
{
  call(::BCryptHash, algorithm,
    static_cast<PUCHAR>(const_cast<void *>(key)),
    static_cast<ULONG>(key_size),
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(data_size),
    static_cast<PUCHAR>(digest),
    static_cast<ULONG>(digest_size)
  );
}


} // namespace __bits


template <typename Algorithm>
hash_t<Algorithm>::hash_t ()
  : ctx_{__bits::context_t::make<Algorithm, false>()}
{}


template <typename Algorithm>
void hash_t<Algorithm>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}


template <typename Algorithm>
void hash_t<Algorithm>::finish (void *digest, size_t size)
{
  ctx_.finish(digest, size);
}


template <typename Algorithm>
void hash_t<Algorithm>::one_shot (const void *data, size_t data_size,
  void *digest, size_t digest_length)
{
  __bits::context_t::hash(
    __bits::context_t::factory<Algorithm, false>(),
    data, data_size,
    digest, digest_length
  );
}

// }}}1

#endif


template class hash_t<md5>;
template class hash_t<sha1>;
template class hash_t<sha256>;
template class hash_t<sha384>;
template class hash_t<sha512>;


} // namespace crypto


__sal_end
