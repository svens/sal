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
  ::CC_MD5_Init(&ctx_);
}

template <>
void hash_t<md5>::update (const void *data, size_t size)
{
  ::CC_MD5_Update(&ctx_, data, size);
}

template <>
void hash_t<md5>::finish (void *result, size_t)
{
  ::CC_MD5_Final(static_cast<uint8_t *>(result), &ctx_);
  ::CC_MD5_Init(&ctx_);
}

template <>
void hash_t<md5>::one_shot (const void *data, size_t data_size,
  void *result, size_t)
{
  ::CC_MD5(data, data_size, static_cast<uint8_t *>(result));
}

// sha1 {{{1

template <>
hash_t<sha1>::hash_t ()
{
  ::CC_SHA1_Init(&ctx_);
}

template <>
void hash_t<sha1>::update (const void *data, size_t size)
{
  ::CC_SHA1_Update(&ctx_, data, size);
}

template <>
void hash_t<sha1>::finish (void *result, size_t)
{
  ::CC_SHA1_Final(static_cast<uint8_t *>(result), &ctx_);
  ::CC_SHA1_Init(&ctx_);
}

template <>
void hash_t<sha1>::one_shot (const void *data, size_t data_size,
  void *result, size_t)
{
  ::CC_SHA1(data, data_size, static_cast<uint8_t *>(result));
}

// sha256 {{{1

template <>
hash_t<sha256>::hash_t ()
{
  ::CC_SHA256_Init(&ctx_);
}

template <>
void hash_t<sha256>::update (const void *data, size_t size)
{
  ::CC_SHA256_Update(&ctx_, data, size);
}

template <>
void hash_t<sha256>::finish (void *result, size_t)
{
  ::CC_SHA256_Final(static_cast<uint8_t *>(result), &ctx_);
  ::CC_SHA256_Init(&ctx_);
}

template <>
void hash_t<sha256>::one_shot (const void *data, size_t data_size,
  void *result, size_t)
{
  ::CC_SHA256(data, data_size, static_cast<uint8_t *>(result));
}

// sha384 {{{1

template <>
hash_t<sha384>::hash_t ()
{
  ::CC_SHA384_Init(&ctx_);
}

template <>
void hash_t<sha384>::update (const void *data, size_t size)
{
  ::CC_SHA384_Update(&ctx_, data, size);
}

template <>
void hash_t<sha384>::finish (void *result, size_t)
{
  ::CC_SHA384_Final(static_cast<uint8_t *>(result), &ctx_);
  ::CC_SHA384_Init(&ctx_);
}

template <>
void hash_t<sha384>::one_shot (const void *data, size_t data_size,
  void *result, size_t)
{
  ::CC_SHA384(data, data_size, static_cast<uint8_t *>(result));
}

// sha512 {{{1

template <>
hash_t<sha512>::hash_t ()
{
  ::CC_SHA512_Init(&ctx_);
}

template <>
void hash_t<sha512>::update (const void *data, size_t size)
{
  ::CC_SHA512_Update(&ctx_, data, size);
}

template <>
void hash_t<sha512>::finish (void *result, size_t)
{
  ::CC_SHA512_Final(static_cast<uint8_t *>(result), &ctx_);
  ::CC_SHA512_Init(&ctx_);
}

template <>
void hash_t<sha512>::one_shot (const void *data, size_t data_size,
  void *result, size_t)
{
  ::CC_SHA512(data, data_size, static_cast<uint8_t *>(result));
}

// }}}

#elif __sal_os_linux

// md5 {{{1

template <>
hash_t<md5>::hash_t ()
{
  ::MD5_Init(&ctx_);
}

template <>
void hash_t<md5>::update (const void *data, size_t size)
{
  ::MD5_Update(&ctx_, data, size);
}

template <>
void hash_t<md5>::finish (void *result, size_t)
{
  ::MD5_Final(static_cast<uint8_t *>(result), &ctx_);
  ::MD5_Init(&ctx_);
}

template <>
void hash_t<md5>::one_shot (const void *data, size_t data_size,
  void *result, size_t)
{
  ::MD5(static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(result)
  );
}

// sha1 {{{1

template <>
hash_t<sha1>::hash_t ()
{
  ::SHA1_Init(&ctx_);
}

template <>
void hash_t<sha1>::update (const void *data, size_t size)
{
  ::SHA1_Update(&ctx_, data, size);
}

template <>
void hash_t<sha1>::finish (void *result, size_t)
{
  ::SHA1_Final(static_cast<uint8_t *>(result), &ctx_);
  ::SHA1_Init(&ctx_);
}

template <>
void hash_t<sha1>::one_shot (const void *data, size_t data_size,
  void *result, size_t)
{
  ::SHA1(static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(result)
  );
}

// sha256 {{{1

template <>
hash_t<sha256>::hash_t ()
{
  ::SHA256_Init(&ctx_);
}

template <>
void hash_t<sha256>::update (const void *data, size_t size)
{
  ::SHA256_Update(&ctx_, data, size);
}

template <>
void hash_t<sha256>::finish (void *result, size_t)
{
  ::SHA256_Final(static_cast<uint8_t *>(result), &ctx_);
  ::SHA256_Init(&ctx_);
}

template <>
void hash_t<sha256>::one_shot (const void *data, size_t data_size,
  void *result, size_t)
{
  ::SHA256(static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(result)
  );
}

// sha384 {{{1

template <>
hash_t<sha384>::hash_t ()
{
  ::SHA384_Init(&ctx_);
}

template <>
void hash_t<sha384>::update (const void *data, size_t size)
{
  ::SHA384_Update(&ctx_, data, size);
}

template <>
void hash_t<sha384>::finish (void *result, size_t)
{
  ::SHA384_Final(static_cast<uint8_t *>(result), &ctx_);
  ::SHA384_Init(&ctx_);
}

template <>
void hash_t<sha384>::one_shot (const void *data, size_t data_size,
  void *result, size_t)
{
  ::SHA384(static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(result)
  );
}

// sha512 {{{1

template <>
hash_t<sha512>::hash_t ()
{
  ::SHA512_Init(&ctx_);
}

template <>
void hash_t<sha512>::update (const void *data, size_t size)
{
  ::SHA512_Update(&ctx_, data, size);
}

template <>
void hash_t<sha512>::finish (void *result, size_t)
{
  ::SHA512_Final(static_cast<uint8_t *>(result), &ctx_);
  ::SHA512_Init(&ctx_);
}

template <>
void hash_t<sha512>::one_shot (const void *data, size_t data_size,
  void *result, size_t)
{
  ::SHA512(static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(result)
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


template <typename Digest> constexpr LPCWSTR digest_id_v = nullptr;
template <> constexpr LPCWSTR digest_id_v<md5> = BCRYPT_MD5_ALGORITHM;
template <> constexpr LPCWSTR digest_id_v<sha1> = BCRYPT_SHA1_ALGORITHM;
template <> constexpr LPCWSTR digest_id_v<sha256> = BCRYPT_SHA256_ALGORITHM;
template <> constexpr LPCWSTR digest_id_v<sha384> = BCRYPT_SHA384_ALGORITHM;
template <> constexpr LPCWSTR digest_id_v<sha512> = BCRYPT_SHA512_ALGORITHM;

template <bool IsHMAC> constexpr DWORD digest_flags_v = 0U;
template <> constexpr DWORD digest_flags_v<true> = BCRYPT_ALG_HANDLE_HMAC_FLAG;


struct digest_factory_t
{
  BCRYPT_ALG_HANDLE handle;

  digest_factory_t (LPCWSTR id, DWORD flags, size_t expected_hash_length)
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

  ~digest_factory_t () noexcept
  {
    ::BCryptCloseAlgorithmProvider(handle, 0);
  }
};


} // namespace


digest_t::digest_t (const digest_t &that)
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


digest_t::~digest_t () noexcept
{
  if (handle)
  {
    call(::BCryptDestroyHash, handle);
  }
}


template <typename Digest, bool IsHMAC>
BCRYPT_ALG_HANDLE digest_t::factory ()
{
  static digest_factory_t factory_
  {
    digest_id_v<Digest>,
    digest_flags_v<IsHMAC>,
    digest_size_v<Digest>
  };
  return factory_.handle;
}

template BCRYPT_ALG_HANDLE digest_t::factory<md5, false> ();
template BCRYPT_ALG_HANDLE digest_t::factory<md5, true> ();
template BCRYPT_ALG_HANDLE digest_t::factory<sha1, false> ();
template BCRYPT_ALG_HANDLE digest_t::factory<sha1, true> ();
template BCRYPT_ALG_HANDLE digest_t::factory<sha256, false> ();
template BCRYPT_ALG_HANDLE digest_t::factory<sha256, true> ();
template BCRYPT_ALG_HANDLE digest_t::factory<sha384, false> ();
template BCRYPT_ALG_HANDLE digest_t::factory<sha384, true> ();
template BCRYPT_ALG_HANDLE digest_t::factory<sha512, false> ();
template BCRYPT_ALG_HANDLE digest_t::factory<sha512, true> ();


template <typename Digest, bool IsHMAC>
BCRYPT_HASH_HANDLE digest_t::make (const void *key, size_t size)
{
  BCRYPT_HASH_HANDLE handle;
  call(::BCryptCreateHash, factory<Digest, IsHMAC>(),
    &handle,
    nullptr, 0,
    static_cast<PUCHAR>(const_cast<void *>(key)),
    static_cast<ULONG>(size),
    BCRYPT_HASH_REUSABLE_FLAG
  );
  return handle;
}

template BCRYPT_HASH_HANDLE digest_t::make<md5, false> (const void *, size_t);
template BCRYPT_HASH_HANDLE digest_t::make<md5, true> (const void *, size_t);
template BCRYPT_HASH_HANDLE digest_t::make<sha1, false> (const void *, size_t);
template BCRYPT_HASH_HANDLE digest_t::make<sha1, true> (const void *, size_t);
template BCRYPT_HASH_HANDLE digest_t::make<sha256, false> (const void *, size_t);
template BCRYPT_HASH_HANDLE digest_t::make<sha256, true> (const void *, size_t);
template BCRYPT_HASH_HANDLE digest_t::make<sha384, false> (const void *, size_t);
template BCRYPT_HASH_HANDLE digest_t::make<sha384, true> (const void *, size_t);
template BCRYPT_HASH_HANDLE digest_t::make<sha512, false> (const void *, size_t);
template BCRYPT_HASH_HANDLE digest_t::make<sha512, true> (const void *, size_t);


void digest_t::update (const void *data, size_t size)
{
  call(::BCryptHashData, handle,
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(size),
    0
  );
}


void digest_t::finish (void *result, size_t size)
{
  call(::BCryptFinishHash, handle,
    static_cast<PUCHAR>(result),
    static_cast<ULONG>(size),
    0
  );
}


void digest_t::hash (BCRYPT_ALG_HANDLE digest,
  const void *data, size_t data_size,
  void *result, size_t result_size)
{
  call(::BCryptHash, digest,
    nullptr, 0,
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(data_size),
    static_cast<PUCHAR>(result),
    static_cast<ULONG>(result_size)
  );
}


void digest_t::hmac (BCRYPT_ALG_HANDLE digest,
  const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t result_size)
{
  call(::BCryptHash, digest,
    static_cast<PUCHAR>(const_cast<void *>(key)),
    static_cast<ULONG>(key_size),
    static_cast<PUCHAR>(const_cast<void *>(data)),
    static_cast<ULONG>(data_size),
    static_cast<PUCHAR>(result),
    static_cast<ULONG>(result_size)
  );
}


} // namespace __bits


// md5 {{{1

template <>
hash_t<md5>::hash_t ()
  : ctx_{__bits::digest_t::make<md5, false>()}
{}

template <>
void hash_t<md5>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hash_t<md5>::finish (void *result, size_t size)
{
  ctx_.finish(result, size);
}

template <>
void hash_t<md5>::one_shot (const void *data, size_t data_size,
  void *result, size_t result_size)
{
  __bits::digest_t::hash(
    __bits::digest_t::factory<md5, false>(),
    data, data_size,
    result, result_size
  );
}

// sha1 {{{1

template <>
hash_t<sha1>::hash_t ()
  : ctx_{__bits::digest_t::make<sha1, false>()}
{}

template <>
void hash_t<sha1>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hash_t<sha1>::finish (void *result, size_t size)
{
  ctx_.finish(result, size);
}

template <>
void hash_t<sha1>::one_shot (const void *data, size_t data_size,
  void *result, size_t result_size)
{
  __bits::digest_t::hash(
    __bits::digest_t::factory<sha1, false>(),
    data, data_size,
    result, result_size
  );
}

// sha256 {{{1

template <>
hash_t<sha256>::hash_t ()
  : ctx_{__bits::digest_t::make<sha256, false>()}
{}

template <>
void hash_t<sha256>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hash_t<sha256>::finish (void *result, size_t size)
{
  ctx_.finish(result, size);
}

template <>
void hash_t<sha256>::one_shot (const void *data, size_t data_size,
  void *result, size_t result_size)
{
  __bits::digest_t::hash(
    __bits::digest_t::factory<sha256, false>(),
    data, data_size,
    result, result_size
  );
}

// sha384 {{{1

template <>
hash_t<sha384>::hash_t ()
  : ctx_{__bits::digest_t::make<sha384, false>()}
{}

template <>
void hash_t<sha384>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hash_t<sha384>::finish (void *result, size_t size)
{
  ctx_.finish(result, size);
}

template <>
void hash_t<sha384>::one_shot (const void *data, size_t data_size,
  void *result, size_t result_size)
{
  __bits::digest_t::hash(
    __bits::digest_t::factory<sha384, false>(),
    data, data_size,
    result, result_size
  );
}

// sha512 {{{1

template <>
hash_t<sha512>::hash_t ()
  : ctx_{__bits::digest_t::make<sha512, false>()}
{}

template <>
void hash_t<sha512>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hash_t<sha512>::finish (void *result, size_t size)
{
  ctx_.finish(result, size);
}

template <>
void hash_t<sha512>::one_shot (const void *data, size_t data_size,
  void *result, size_t result_size)
{
  __bits::digest_t::hash(
    __bits::digest_t::factory<sha512, false>(),
    data, data_size,
    result, result_size
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
