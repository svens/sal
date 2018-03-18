#include <sal/__bits/platform_sdk.hpp>
#include <sal/crypto/key.hpp>

#if __sal_os_macos //{{{1
  #include <Security/SecItem.h>
#elif __sal_os_windows //{{{1
  #include <sal/crypto/hash.hpp>
#endif //}}}1


__sal_begin


namespace crypto {


#if __sal_os_macos // {{{1


namespace {

key_algorithm init (SecKeyRef key, size_t *block_size) noexcept
{
  if (key)
  {
    unique_ref<CFDictionaryRef> dir = ::SecKeyCopyAttributes(key);
    CFTypeRef v;

    // block_size
    if (::CFDictionaryGetValueIfPresent(dir.ref, kSecAttrKeySizeInBits, &v))
    {
      ::CFNumberGetValue(static_cast<CFNumberRef>(v),
        kCFNumberSInt64Type,
        block_size
      );
      *block_size /= 8;
    }

    // type
    if (::CFDictionaryGetValueIfPresent(dir.ref, kSecAttrKeyType, &v))
    {
      if (::CFEqual(v, kSecAttrKeyTypeRSA))
      {
        return key_algorithm::rsa;
      }
    }
  }
  return key_algorithm::opaque;
}

} // namespace


public_key_t::public_key_t (__bits::public_key_t &&that) noexcept
  : impl_(std::move(that))
{
  algorithm_ = init(impl_.ref, &block_size_);
}


private_key_t::private_key_t (__bits::private_key_t &&that) noexcept
  : impl_(std::move(that))
{
  algorithm_ = init(impl_.ref, &block_size_);
}


namespace {

inline unique_ref<CFDataRef> make_data (const void *ptr, size_t size) noexcept
{
  return ::CFDataCreateWithBytesNoCopy(nullptr,
    static_cast<const uint8_t *>(ptr),
    size,
    kCFAllocatorNull
  );
}

inline auto to_algorithm (key_algorithm type, size_t digest_type) noexcept
{
  if (type == key_algorithm::rsa)
  {
    switch (digest_type)
    {
      case __bits::digest_type_v<__bits::sha1_t>:
        return kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA1;
      case __bits::digest_type_v<__bits::sha256_t>:
        return kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA256;
      case __bits::digest_type_v<__bits::sha384_t>:
        return kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA384;
      case __bits::digest_type_v<__bits::sha512_t>:
        return kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA512;
    }
  }
  return SecKeyAlgorithm{};
}

} // namespace


size_t private_key_t::sign (size_t digest_type,
  const void *data, size_t data_size,
  void *signature, size_t signature_size,
  std::error_code &error) noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  auto algorithm = to_algorithm(algorithm_, digest_type);
  if (!algorithm)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return {};
  }

  unique_ref<CFErrorRef> status;
  unique_ref<CFDataRef> sig = ::SecKeyCreateSignature(impl_.ref,
    algorithm,
    make_data(data, data_size).ref,
    &status.ref);

  if (sig)
  {
    size_t sig_size = ::CFDataGetLength(sig.ref);
    if (signature_size >= sig_size)
    {
      auto sig_ptr = ::CFDataGetBytePtr(sig.ref);
      std::uninitialized_copy(sig_ptr, sig_ptr + sig_size,
        static_cast<uint8_t *>(signature)
      );
      error.clear();
      return sig_size;
    }

    error = std::make_error_code(std::errc::result_out_of_range);
    return {};
  }

  error.assign(::CFErrorGetCode(status.ref), category());
  return {};
}


bool public_key_t::verify_signature (size_t digest_type,
  const void *data, size_t data_size,
  const void *signature, size_t signature_size,
  std::error_code &error) noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  auto algorithm = to_algorithm(algorithm_, digest_type);
  if (!algorithm)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return {};
  }

  unique_ref<CFErrorRef> status;
  auto result = ::SecKeyVerifySignature(impl_.ref,
    algorithm,
    make_data(data, data_size).ref,
    make_data(signature, signature_size).ref,
    &status.ref
  );

  if (status.ref == nullptr
    || ::CFErrorGetCode(status.ref) == errSecVerifyFailed)
  {
    error.clear();
  }
  else
  {
    error.assign(::CFErrorGetCode(status.ref), category());
  }

  return result;
}


#elif __sal_os_linux //{{{1


namespace {

key_algorithm init (EVP_PKEY *key, size_t *block_size) noexcept
{
  auto algorithm = key_algorithm::opaque;
  if (key)
  {
    *block_size = EVP_PKEY_size(key);
    switch (EVP_PKEY_id(key))
    {
      case EVP_PKEY_RSA:
        algorithm = key_algorithm::rsa;
        break;
    }
  }
  return algorithm;
}

} // namespace


public_key_t::public_key_t (__bits::public_key_t &&that) noexcept
  : impl_(std::move(that))
{
  algorithm_ = init(impl_.ref, &block_size_);
}


private_key_t::private_key_t (__bits::private_key_t &&that) noexcept
  : impl_(std::move(that))
{
  algorithm_ = init(impl_.ref, &block_size_);
}


namespace {

inline const EVP_MD *to_algorithm (size_t digest) noexcept
{
  switch (digest)
  {
    case __bits::digest_type_v<__bits::sha1_t>:
      return EVP_sha1();
    case __bits::digest_type_v<__bits::sha256_t>:
      return EVP_sha256();
    case __bits::digest_type_v<__bits::sha384_t>:
      return EVP_sha384();
    case __bits::digest_type_v<__bits::sha512_t>:
      return EVP_sha512();
  }
  return nullptr;
}

} // namespace


size_t private_key_t::sign (size_t digest_type,
  const void *data, size_t data_size,
  void *signature, size_t signature_size,
  std::error_code &error) noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  bool invalid_argument = true;
  if (auto algorithm = to_algorithm(digest_type))
  {
#if OPENSSL_VERSION_NUMBER < 0x10100000
    EVP_MD_CTX ctx_buf, *ctx = &ctx_buf;
    EVP_MD_CTX_init(&ctx_buf);
#else
    auto ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
      return {};
    }
#endif

    if (EVP_DigestSignInit(ctx, nullptr, algorithm, nullptr, impl_.ref) == 1
      && EVP_DigestSignUpdate(ctx, data, data_size) == 1)
    {
      size_t result_size;
      if (EVP_DigestSignFinal(ctx, nullptr, &result_size) == 1)
      {
        if (signature_size >= result_size)
        {
          EVP_DigestSignFinal(ctx,
            static_cast<uint8_t *>(signature),
            &signature_size
          );
          error.clear();
        }
        else
        {
          error = std::make_error_code(std::errc::result_out_of_range);
        }
        invalid_argument = false;
      }
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000
    EVP_MD_CTX_cleanup(ctx);
#else
    EVP_MD_CTX_free(ctx);
#endif
  }

  if (invalid_argument)
  {
    error = std::make_error_code(std::errc::invalid_argument);
  }

  return signature_size;
}


bool public_key_t::verify_signature (size_t digest_type,
  const void *data, size_t data_size,
  const void *signature, size_t signature_size,
  std::error_code &error) noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  bool success = false, valid_signature = false;
  if (auto algorithm = to_algorithm(digest_type))
  {
#if OPENSSL_VERSION_NUMBER < 0x10100000
    EVP_MD_CTX ctx_buf, *ctx = &ctx_buf;
    EVP_MD_CTX_init(&ctx_buf);
#else
    auto ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
      return {};
    }
#endif

    if (EVP_DigestVerifyInit(ctx, nullptr, algorithm, nullptr, impl_.ref) == 1
      && EVP_DigestVerifyUpdate(ctx, data, data_size) == 1)
    {
      valid_signature = EVP_DigestVerifyFinal(ctx,
        static_cast<uint8_t *>(const_cast<void *>(signature)),
        signature_size
      );
      success = true;
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000
    EVP_MD_CTX_cleanup(ctx);
#else
    EVP_MD_CTX_free(ctx);
#endif
  }

  if (success)
  {
    error.clear();
  }
  else
  {
    error = std::make_error_code(std::errc::invalid_argument);
  }

  return valid_signature;
}


#elif __sal_os_windows // {{{1


namespace {


key_algorithm init (BCRYPT_KEY_HANDLE key, size_t *block_size) noexcept
{
  if (!key)
  {
    return key_algorithm::opaque;
  }

  // block_size
  DWORD int_buf;
  ULONG buf_size;
  auto status = ::BCryptGetProperty(key,
    BCRYPT_BLOCK_LENGTH,
    reinterpret_cast<PUCHAR>(&int_buf),
    sizeof(int_buf),
    &buf_size,
    0
  );
  if (status != ERROR_SUCCESS)
  {
    return key_algorithm::opaque;
  }
  *block_size = int_buf;

  // type
  wchar_t wchar_buf[256];
  status = ::BCryptGetProperty(key,
    BCRYPT_ALGORITHM_NAME,
    reinterpret_cast<PUCHAR>(wchar_buf),
    sizeof(wchar_buf),
    &buf_size,
    0
  );
  if (status == STATUS_SUCCESS)
  {
    reinterpret_cast<char *>(wchar_buf)[buf_size] = '\0';
    if (wcscmp(wchar_buf, BCRYPT_RSA_ALGORITHM) == 0)
    {
      return key_algorithm::rsa;
    }
  }

  return key_algorithm::opaque;
}


key_algorithm init (NCRYPT_KEY_HANDLE key, size_t *block_size) noexcept
{
  if (!key)
  {
    return key_algorithm::opaque;
  }

  // block_size
  DWORD int_buf;
  ULONG buf_size;
  auto status = ::NCryptGetProperty(key,
    NCRYPT_BLOCK_LENGTH_PROPERTY,
    reinterpret_cast<PUCHAR>(&int_buf),
    sizeof(int_buf),
    &buf_size,
    0
  );
  if (!NT_SUCCESS(status))
  {
    return key_algorithm::opaque;
  }
  *block_size = int_buf;

  // type
  wchar_t wchar_buf[256];
  status = ::NCryptGetProperty(key,
    NCRYPT_ALGORITHM_GROUP_PROPERTY,
    reinterpret_cast<PUCHAR>(wchar_buf),
    sizeof(wchar_buf),
    &buf_size,
    0
  );
  if (NT_SUCCESS(status))
  {
    reinterpret_cast<char *>(wchar_buf)[buf_size] = '\0';
    if (wcscmp(wchar_buf, NCRYPT_RSA_ALGORITHM_GROUP) == 0)
    {
      return key_algorithm::rsa;
    }
  }

  return key_algorithm::opaque;
}


} // namespace


public_key_t::public_key_t (__bits::public_key_t &&that) noexcept
  : impl_(std::move(that))
{
  algorithm_ = init(impl_.ref, &block_size_);
}


private_key_t::private_key_t (__bits::private_key_t &&that) noexcept
  : impl_(std::move(that))
{
  algorithm_ = init(impl_.ref, &block_size_);
}


namespace {


template <typename Digest>
inline DWORD make_digest (const uint8_t *first, const uint8_t *last,
  uint8_t *buf, DWORD size) noexcept
{
  constexpr auto digest_size = static_cast<DWORD>(hash_t<Digest>::digest_size);
  if (size >= digest_size)
  {
    try
    {
      hash_t<Digest>::one_shot(first, last, buf, buf + digest_size);
      return digest_size;
    }
    catch (...)
    {}
  }
  return 0;
}


wchar_t *digest_and_algorithm (size_t digest_type,
  const uint8_t *first, const uint8_t *last,
  uint8_t *digest_buf, DWORD *digest_size) noexcept
{
  switch (digest_type)
  {
    case __bits::digest_type_v<__bits::sha1_t>:
      *digest_size = make_digest<sha1>(first, last, digest_buf, *digest_size);
      return NCRYPT_SHA1_ALGORITHM;

    case __bits::digest_type_v<__bits::sha256_t>:
      *digest_size = make_digest<sha256>(first, last, digest_buf, *digest_size);
      return NCRYPT_SHA256_ALGORITHM;

    case __bits::digest_type_v<__bits::sha384_t>:
      *digest_size = make_digest<sha384>(first, last, digest_buf, *digest_size);
      return NCRYPT_SHA384_ALGORITHM;

    case __bits::digest_type_v<__bits::sha512_t>:
      *digest_size = make_digest<sha512>(first, last, digest_buf, *digest_size);
      return NCRYPT_SHA512_ALGORITHM;

    default:
      return {};
  }
}


} // namespace


size_t private_key_t::sign (size_t digest_type,
  const void *data, size_t data_size,
  void *signature, size_t signature_size,
  std::error_code &error) noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  uint8_t digest_buf[1024];
  DWORD digest_size = sizeof(digest_buf);

  BCRYPT_PKCS1_PADDING_INFO padding_info;
  padding_info.pszAlgId = digest_and_algorithm(digest_type,
    static_cast<const uint8_t *>(data),
    static_cast<const uint8_t *>(data) + data_size,
    digest_buf,
    &digest_size
  );
  if (!padding_info.pszAlgId || !digest_size)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return {};
  }

  auto result_size = static_cast<DWORD>(signature_size);
  auto status = ::NCryptSignHash(impl_.ref,
    &padding_info,
    digest_buf,
    digest_size,
    static_cast<PBYTE>(signature),
    result_size,
    &result_size,
    BCRYPT_PAD_PKCS1
  );

  if (status == ERROR_SUCCESS)
  {
    error.clear();
  }
  else if (status == NTE_BUFFER_TOO_SMALL)
  {
    error = std::make_error_code(std::errc::result_out_of_range);
  }
  else
  {
    error.assign(status, category());
  }

  return result_size;
}


bool public_key_t::verify_signature (size_t digest_type,
  const void *data, size_t data_size,
  const void *signature, size_t signature_size,
  std::error_code &error) noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  uint8_t digest_buf[1024];
  DWORD digest_size = sizeof(digest_buf);

  BCRYPT_PKCS1_PADDING_INFO padding_info;
  padding_info.pszAlgId = digest_and_algorithm(digest_type,
    static_cast<const uint8_t *>(data),
    static_cast<const uint8_t *>(data) + data_size,
    digest_buf,
    &digest_size
  );
  if (!padding_info.pszAlgId || !digest_size)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return {};
  }

  auto status = ::BCryptVerifySignature(impl_.ref,
    &padding_info,
    digest_buf,
    digest_size,
    static_cast<PUCHAR>(const_cast<void *>(signature)),
    static_cast<DWORD>(signature_size),
    BCRYPT_PAD_PKCS1
  );

  if (status == STATUS_SUCCESS || status == STATUS_INVALID_SIGNATURE)
  {
    error.clear();
    return status == STATUS_SUCCESS;
  }

  error.assign(status, category());
  return {};
}


#endif // }}}1


} // namespace crypto


__sal_end
