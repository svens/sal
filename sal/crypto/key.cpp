#include <sal/__bits/platform_sdk.hpp>
#include <sal/crypto/key.hpp>
#include <sal/assert.hpp>

#if __sal_os_macos //{{{1
  #include <Security/SecItem.h>
  #include <memory>
#elif __sal_os_linux //{{{1
  #include <openssl/err.h>
#elif __sal_os_windows //{{{1
  #include <sal/buf_ptr.hpp>
  #include <sal/crypto/hash.hpp>
#endif //}}}1


__sal_begin


namespace crypto {


#if __sal_os_macos // {{{1


namespace {

key_type init (SecKeyRef key, size_t *block_size) noexcept
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
        return key_type::rsa;
      }
    }
  }
  return key_type::opaque;
}

} // namespace


public_key_t::public_key_t (__bits::public_key_t &&that) noexcept
  : impl_(std::move(that))
{
  type_ = init(impl_.ref, &block_size_);
}


private_key_t::private_key_t (__bits::private_key_t &&that) noexcept
  : impl_(std::move(that))
{
  type_ = init(impl_.ref, &block_size_);
}


namespace {

inline unique_ref<CFDataRef> make_data (const uint8_t *first,
  const uint8_t *last) noexcept
{
  return ::CFDataCreateWithBytesNoCopy(nullptr,
    first, last - first,
    kCFAllocatorNull
  );
}

inline auto to_algorithm (key_type type, sign_digest_type digest) noexcept
{
  if (type == key_type::rsa)
  {
    switch (digest)
    {
      case sign_digest_type::sha1:
        return kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA1;
      case sign_digest_type::sha256:
        return kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA256;
      case sign_digest_type::sha384:
        return kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA384;
      case sign_digest_type::sha512:
        return kSecKeyAlgorithmRSASignatureMessagePKCS1v15SHA512;
    }
  }
  return SecKeyAlgorithm{};
}

} // namespace


uint8_t *private_key_t::sign (sign_digest_type digest,
  const uint8_t *first, const uint8_t *last,
  uint8_t *signature_first, uint8_t *signature_last,
  std::error_code &error) noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return signature_first;
  }

  auto algorithm = to_algorithm(type_, digest);
  if (!algorithm)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return signature_first;
  }

  unique_ref<CFErrorRef> status;
  unique_ref<CFDataRef> signature = ::SecKeyCreateSignature(impl_.ref,
    algorithm,
    make_data(first, last).ref,
    &status.ref);

  if (signature)
  {
    auto s = ::CFDataGetLength(signature.ref);
    if (signature_last - signature_first >= s)
    {
      auto p = CFDataGetBytePtr(signature.ref);
      signature_last = std::uninitialized_copy(p, p + s, signature_first);
      error.clear();
    }
    else
    {
      error = std::make_error_code(std::errc::result_out_of_range);
    }
  }
  else
  {
    error.assign(::CFErrorGetCode(status.ref), category());
  }

  return signature_last;
}


bool public_key_t::verify_signature (sign_digest_type digest,
  const uint8_t *first, const uint8_t *last,
  const uint8_t *signature_first, const uint8_t *signature_last,
  std::error_code &error) noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  auto algorithm = to_algorithm(type_, digest);
  if (!algorithm)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return {};
  }

  unique_ref<CFErrorRef> status;
  auto result = ::SecKeyVerifySignature(impl_.ref,
    algorithm,
    make_data(first, last).ref,
    make_data(signature_first, signature_last).ref,
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

key_type init (EVP_PKEY *key, size_t *block_size) noexcept
{
  if (!key)
  {
    return key_type::opaque;
  }

  *block_size = EVP_PKEY_size(key);

  switch (EVP_PKEY_id(key))
  {
    case EVP_PKEY_RSA:
      return key_type::rsa;
  }

  return key_type::opaque;
}

} // namespace


public_key_t::public_key_t (__bits::public_key_t &&that) noexcept
  : impl_(std::move(that))
{
  type_ = init(impl_.ref, &block_size_);
}


private_key_t::private_key_t (__bits::private_key_t &&that) noexcept
  : impl_(std::move(that))
{
  type_ = init(impl_.ref, &block_size_);
}


namespace {

inline const EVP_MD *to_algorithm (sign_digest_type digest) noexcept
{
  switch (digest)
  {
    case sign_digest_type::sha1:
      return EVP_sha1();
    case sign_digest_type::sha256:
      return EVP_sha256();
    case sign_digest_type::sha384:
      return EVP_sha384();
    case sign_digest_type::sha512:
      return EVP_sha512();
  }
  return nullptr;
}

} // namespace


uint8_t *private_key_t::sign (sign_digest_type digest,
  const uint8_t *first, const uint8_t *last,
  uint8_t *signature_first, uint8_t *signature_last,
  std::error_code &error) noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return signature_first;
  }

  auto algorithm = to_algorithm(digest);
  if (!algorithm)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return signature_first;
  }

  EVP_MD_CTX md_ctx;
  EVP_MD_CTX_init(&md_ctx);

  auto status = EVP_DigestSignInit(&md_ctx,
    nullptr,
    algorithm,
    nullptr,
    impl_.ref
  );

  if (status == 1)
  {
    status = EVP_DigestSignUpdate(&md_ctx, first, last - first);
  }

  if (status == 1)
  {
    size_t signature_size = signature_last - signature_first;
    status = EVP_DigestSignFinal(&md_ctx, signature_first, &signature_size);
    signature_last = signature_first + signature_size;
  }

  if (status == 1)
  {
    error.clear();
  }
  else
  {
    error = std::make_error_code(std::errc::invalid_argument);
  }

  EVP_MD_CTX_cleanup(&md_ctx);
  return signature_last;
}


bool public_key_t::verify_signature (sign_digest_type digest,
  const uint8_t *first, const uint8_t *last,
  const uint8_t *signature_first, const uint8_t *signature_last,
  std::error_code &error) noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  auto algorithm = to_algorithm(digest);
  if (!algorithm)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return {};
  }

  EVP_MD_CTX md_ctx;
  EVP_MD_CTX_init(&md_ctx);

  auto status = EVP_DigestVerifyInit(&md_ctx,
    nullptr,
    algorithm,
    nullptr,
    impl_.ref
  );

  if (status == 1)
  {
    status = EVP_DigestVerifyUpdate(&md_ctx, first, last - first);
  }

  bool result = false;
  if (status == 1)
  {
    status = EVP_DigestVerifyFinal(&md_ctx,
      const_cast<uint8_t *>(signature_first),
      signature_last - signature_first
    );
    if (status == 1)
    {
      result = true;
      status = 0;
    }
  }

  if (!status)
  {
    error.clear();
  }
  else
  {
    error = std::make_error_code(std::errc::invalid_argument);
  }

  EVP_MD_CTX_cleanup(&md_ctx);
  return result;
}


#elif __sal_os_windows // {{{1


namespace {


key_type init (BCRYPT_KEY_HANDLE key, size_t *block_size) noexcept
{
  if (!key)
  {
    return key_type::opaque;
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
    return key_type::opaque;
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
      return key_type::rsa;
    }
  }

  return key_type::opaque;
}


key_type init (NCRYPT_KEY_HANDLE key, size_t *block_size) noexcept
{
  if (!key)
  {
    return key_type::opaque;
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
    return key_type::opaque;
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
      return key_type::rsa;
    }
  }

  return key_type::opaque;
}


} // namespace


public_key_t::public_key_t (__bits::public_key_t &&that) noexcept
  : impl_(std::move(that))
{
  type_ = init(impl_.ref, &block_size_);
}


private_key_t::private_key_t (__bits::private_key_t &&that) noexcept
  : impl_(std::move(that))
{
  type_ = init(impl_.ref, &block_size_);
}


namespace {


template <typename Digest>
inline DWORD make_digest (const uint8_t *first, const uint8_t *last,
  uint8_t *buf, DWORD size) noexcept
{
  constexpr auto digest_size = static_cast<DWORD>(hash_t<Digest>::digest_size());
  if (size >= digest_size)
  {
    try
    {
      hash_t<Digest>::one_shot(
        make_buf(first, last - first),
        make_buf(buf, digest_size)
      );
      return digest_size;
    }
    catch (...)
    {}
  }
  return 0;
}


wchar_t *digest_and_algorithm (sign_digest_type algorithm,
  const uint8_t *first, const uint8_t *last,
  uint8_t *digest_buf, DWORD *digest_size) noexcept
{
  switch (algorithm)
  {
    case sign_digest_type::sha1:
      *digest_size = make_digest<sha1>(first, last, digest_buf, *digest_size);
      return NCRYPT_SHA1_ALGORITHM;

    case sign_digest_type::sha256:
      *digest_size = make_digest<sha256>(first, last, digest_buf, *digest_size);
      return NCRYPT_SHA256_ALGORITHM;

    case sign_digest_type::sha384:
      *digest_size = make_digest<sha384>(first, last, digest_buf, *digest_size);
      return NCRYPT_SHA384_ALGORITHM;

    case sign_digest_type::sha512:
      *digest_size = make_digest<sha512>(first, last, digest_buf, *digest_size);
      return NCRYPT_SHA512_ALGORITHM;

    default:
      return {};
  }
}


} // namespace


uint8_t *private_key_t::sign (sign_digest_type digest,
  const uint8_t *first, const uint8_t *last,
  uint8_t *signature_first, uint8_t *signature_last,
  std::error_code &error) noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return signature_first;
  }

  uint8_t digest_buf[1024];
  DWORD digest_size = sizeof(digest_buf);

  BCRYPT_PKCS1_PADDING_INFO padding_info;
  padding_info.pszAlgId = digest_and_algorithm(digest,
    first,
    last,
    digest_buf,
    &digest_size
  );
  if (!padding_info.pszAlgId || !digest_size)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return signature_first;
  }

  auto signature_size = static_cast<DWORD>(signature_last - signature_first);
  auto status = ::NCryptSignHash(impl_.ref,
    &padding_info,
    digest_buf,
    digest_size,
    signature_first,
    signature_size,
    &signature_size,
    BCRYPT_PAD_PKCS1
  );

  if (status == ERROR_SUCCESS)
  {
    signature_last = signature_first + signature_size;
    error.clear();
  }
  else
  {
    error.assign(status, category());
  }

  return signature_last;
}


bool public_key_t::verify_signature (sign_digest_type digest,
  const uint8_t *first, const uint8_t *last,
  const uint8_t *signature_first, const uint8_t *signature_last,
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
  padding_info.pszAlgId = digest_and_algorithm(digest,
    first,
    last,
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
    const_cast<PUCHAR>(signature_first),
    static_cast<DWORD>(signature_last - signature_first),
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
