#include <sal/__bits/platform_sdk.hpp>
#include <sal/crypto/key.hpp>
#include <sal/assert.hpp>

#include <iostream>

#if __sal_os_macos //{{{1
  #include <Security/SecItem.h>
  #include <memory>

#if 0
  #if !defined(__apple_build_version__)
    #define availability(...) /**/
  #endif
  #include <CoreFoundation/CFNumber.h>
  #include <CoreFoundation/CFURL.h>
  #include <Security/SecCertificateOIDs.h>
#endif

#elif __sal_os_linux //{{{1
  #include <openssl/asn1.h>
  #include <openssl/x509v3.h>
#endif //}}}1


__sal_begin


namespace crypto {


#if __sal_os_macos // {{{1


namespace {

key_type init (SecKeyRef key, size_t *block_size) noexcept
{
  if (!key)
  {
    return key_type::opaque;
  }

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


#endif // }}}1


} // namespace crypto


__sal_end
