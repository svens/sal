#include <sal/crypto/certificate.hpp>
#include <sal/crypto/error.hpp>
#include <sal/net/ip/__bits/inet.hpp>
#include <sal/assert.hpp>
#include <cstdlib>

#if __sal_os_macos //{{{1
  #if !defined(__apple_build_version__)
    #define availability(...) /**/
  #endif
  #include <CoreFoundation/CoreFoundation.h>
  #include <Security/Security.h>
  #include <dlfcn.h>
  #include <mutex>
#elif __sal_os_linux //{{{1
  #include <openssl/asn1.h>
  #include <openssl/err.h>
  #include <openssl/evp.h>
  #include <openssl/pem.h>
  #include <openssl/pkcs12.h>
  #include <openssl/x509v3.h>
  #include <mutex>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
#endif //}}}1


__sal_begin


namespace crypto {


namespace {


std::string normalized_ip_string (const uint8_t *first, size_t size)
{
  char buf[128] = { 0 };
  if (size == 4)
  {
    auto a = *reinterpret_cast<const in_addr *>(first);
    net::ip::__bits::inet_ntop(a, buf, sizeof(buf));
  }
  else if (size == 16)
  {
    auto a = *reinterpret_cast<const in6_addr *>(first);
    net::ip::__bits::inet_ntop(a, buf, sizeof(buf));
  }
  return buf;
}


std::vector<uint8_t> calculate_digest (
  const uint8_t *data, size_t size,
  std::vector<uint8_t>(*fn)(const uint8_t *, size_t),
  std::error_code &error) noexcept
{
  try
  {
    return fn(data, size);
  }

  // LCOV_EXCL_START
  catch (const std::system_error &e)
  {
    error = e.code();
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  return {};
  // LCOV_EXCL_STOP
}


inline std::string rdn_escape (const std::string &value)
{
  std::string result;
  for (auto ch: value)
  {
    if (ch == ','
      || ch == '='
      || ch == '+'
      || ch == '<'
      || ch == '>'
      || ch == '#'
      || ch == ';')
    {
      result += '\\';
    }
    result += ch;
  }
  return result;
}


} // namespace


certificate_t certificate_t::from_pem (const uint8_t *first, const uint8_t *last,
  std::error_code &error) noexcept
{
  uint8_t der[16 * 1024];
  if (auto der_end = __bits::pem_to_der(first, last, der, der + sizeof(der)))
  {
    return from_der(der, der_end, error);
  }
  error = std::make_error_code(std::errc::invalid_argument);
  return {};
}


memory_writer_t &operator<< (memory_writer_t &writer,
  const certificate_t::distinguished_name_format_t &rdn)
{
  for (const auto &name: rdn.rdn)
  {
    writer
      << alias_or_oid(name.first)
      << rdn.assign
      << rdn_escape(name.second)
      << rdn.separator;
  }
  writer.first -= rdn.separator.size();

  return writer;
}


#if __sal_os_macos // {{{1


namespace {


template <size_t N>
inline const char *c_str (CFTypeRef s, char (&buf)[N]) noexcept
{
  constexpr auto encoding = kCFStringEncodingUTF8;

  auto cfstr = static_cast<CFStringRef>(s);
  if (auto p = ::CFStringGetCStringPtr(cfstr, encoding))
  {
    return p;
  }
  // LCOV_EXCL_START
  ::CFStringGetCString(cfstr, buf, N, encoding);
  return static_cast<const char *>(&buf[0]);
  // LCOV_EXCL_STOP
}


unique_ref<CFArrayRef> copy_values (SecCertificateRef cert, CFTypeRef oid,
  std::error_code &error) noexcept
{
  if (cert)
  {
    unique_ref<CFArrayRef> keys = ::CFArrayCreate(nullptr,
      &oid, 1,
      &kCFTypeArrayCallBacks
    );
    unique_ref<CFDictionaryRef> dir = ::SecCertificateCopyValues(cert,
      keys.ref,
      nullptr
    );

    CFTypeRef values;
    if (::CFDictionaryGetValueIfPresent(dir.ref, oid, &values))
    {
      if (::CFDictionaryGetValueIfPresent(static_cast<CFDictionaryRef>(values),
          kSecPropertyKeyValue,
          &values))
      {
        return static_cast<CFArrayRef>(::CFRetain(values));
      }
    }

    error.clear();
  }
  else
  {
    error = std::make_error_code(std::errc::bad_address);
  }
  return {};
}


auto to_distinguished_name (SecCertificateRef cert, CFTypeRef oid,
  std::error_code &error) noexcept
{
  certificate_t::distinguished_name_t result;

  if (auto values = copy_values(cert, oid, error))
  {
    try
    {
      for (auto i = 0;  i != ::CFArrayGetCount(values.ref);  ++i)
      {
        auto key = (CFDictionaryRef)::CFArrayGetValueAtIndex(values.ref, i);
        char buf[1024];
        result.emplace_back(
          c_str(::CFDictionaryGetValue(key, kSecPropertyKeyLabel), buf),
          c_str(::CFDictionaryGetValue(key, kSecPropertyKeyValue), buf)
        );
      }

      error.clear();
    }

    // LCOV_EXCL_START
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
      result.clear();
    }
    // LCOV_EXCL_STOP
  }

  return result;
}


auto to_distinguished_name (SecCertificateRef cert,
  CFTypeRef oid,
  const oid_t &filter_oid,
  std::error_code &error) noexcept
{
  certificate_t::distinguished_name_t result;

  if (auto values = copy_values(cert, oid, error))
  {
    try
    {
      unique_ref<CFStringRef> filter = ::CFStringCreateWithBytesNoCopy(
        nullptr,
        reinterpret_cast<const UInt8 *>(filter_oid.data()),
        filter_oid.size(),
        kCFStringEncodingUTF8,
        false,
        kCFAllocatorNull
      );

      for (auto i = 0;  i != ::CFArrayGetCount(values.ref);  ++i)
      {
        auto key = (CFDictionaryRef)::CFArrayGetValueAtIndex(values.ref, i);
        auto label = (CFStringRef)::CFDictionaryGetValue(key, kSecPropertyKeyLabel);
        if (::CFEqual(label, filter.ref))
        {
          char buf[1024];
          result.emplace_back(
            c_str(label, buf),
            c_str(::CFDictionaryGetValue(key, kSecPropertyKeyValue), buf)
          );
        }
      }

      error.clear();
    }

    // LCOV_EXCL_START
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
      result.clear();
    }
    // LCOV_EXCL_STOP
  }

  return result;
}


sal::time_t to_time (SecCertificateRef cert, CFTypeRef oid,
  std::error_code &error) noexcept
{
  if (auto value = copy_values(cert, oid, error))
  {
    CFAbsoluteTime time;
    ::CFNumberGetValue((CFNumberRef)value.ref, kCFNumberDoubleType, &time);

    error.clear();
    return sal::clock_t::from_time_t(time + kCFAbsoluteTimeIntervalSince1970);
  }

  return {};
}


std::vector<uint8_t> key_identifier (SecCertificateRef cert, CFTypeRef oid,
  std::error_code &error) noexcept
{
  std::vector<uint8_t> result;
  if (auto values = copy_values(cert, oid, error))
  {
    try
    {
      auto data = (CFDataRef)::CFDictionaryGetValue(
        (CFDictionaryRef)::CFArrayGetValueAtIndex(values.ref, 1),
        kSecPropertyKeyValue
      );
      auto content = ::CFDataGetBytePtr(data);
      result.assign(content, content + ::CFDataGetLength(data));
      error.clear();
    }

    // LCOV_EXCL_START
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
    }
    // LCOV_EXCL_STOP
  }

  return result;
}


inline unique_ref<CFDataRef> data (const uint8_t *first, const uint8_t *last)
  noexcept
{
  return ::CFDataCreateWithBytesNoCopy(nullptr,
    first, last - first,
    kCFAllocatorNull
  );
}


} // namespace


bool certificate_t::issued_by (const certificate_t &issuer,
  std::error_code &error) const noexcept
{
  if (impl_ && issuer.impl_)
  {
    error.clear();

    unique_ref<CFDataRef>
      this_issuer = ::SecCertificateCopyNormalizedIssuerSequence(impl_.ref),
      issuer_subject = SecCertificateCopyNormalizedSubjectSequence(issuer.impl_.ref);

    auto size_1 = ::CFDataGetLength(issuer_subject.ref);
    auto size_2 = ::CFDataGetLength(this_issuer.ref);
    if (size_1 != size_2)
    {
      return false;
    }

    auto data_1 = ::CFDataGetBytePtr(issuer_subject.ref);
    auto data_2 = ::CFDataGetBytePtr(this_issuer.ref);
    return std::equal(data_1, data_1 + size_1, data_2);
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


certificate_t certificate_t::from_der (const uint8_t *first, const uint8_t *last,
  std::error_code &error) noexcept
{
  auto cert = ::SecCertificateCreateWithData(nullptr, data(first, last).ref);
  if (cert)
  {
    error.clear();
  }
  else
  {
    error = std::make_error_code(std::errc::invalid_argument);
  }
  return __bits::certificate_t{cert};
}


bool certificate_t::operator== (const certificate_t &that) const noexcept
{
  if (impl_.ref && that.impl_.ref)
  {
    return ::CFEqual(impl_.ref, that.impl_.ref);
  }
  else if (!impl_.ref && !that.impl_.ref)
  {
    return true;
  }
  return false;
}


uint8_t *certificate_t::to_der (uint8_t *first, uint8_t *last,
  std::error_code &error) const noexcept
{
  if (unique_ref<CFDataRef> data = ::SecCertificateCopyData(impl_.ref))
  {
    auto size = ::CFDataGetLength(data.ref);
    if (last - first >= size)
    {
      auto der = ::CFDataGetBytePtr(data.ref);
      error.clear();
      return std::uninitialized_copy(der, der + size, first);
    }
    error = std::make_error_code(std::errc::result_out_of_range);
  }
  else
  {
    error = std::make_error_code(std::errc::bad_address);
  }
  return {};
}


std::vector<uint8_t> certificate_t::to_der (std::error_code &error)
  const noexcept
{
  if (unique_ref<CFDataRef> data = ::SecCertificateCopyData(impl_.ref))
  {
    try
    {
      auto der = ::CFDataGetBytePtr(data.ref);
      return std::vector<uint8_t>(der, der + ::CFDataGetLength(data.ref));
    }

    // LCOV_EXCL_START
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
    }
    // LCOV_EXCL_STOP
  }
  else
  {
    error = std::make_error_code(std::errc::bad_address);
  }
  return {};
}


int certificate_t::version () const noexcept
{
  std::error_code ignored;
  if (auto value = copy_values(impl_.ref, kSecOIDX509V1Version, ignored))
  {
    char buf[16];
    return std::atoi(c_str(value.ref, buf));
  }
  return 0;
}


sal::time_t certificate_t::not_before (std::error_code &error) const noexcept
{
  return to_time(impl_.ref, kSecOIDX509V1ValidityNotBefore, error);
}


sal::time_t certificate_t::not_after (std::error_code &error) const noexcept
{
  return to_time(impl_.ref, kSecOIDX509V1ValidityNotAfter, error);
}


std::vector<uint8_t> certificate_t::serial_number (std::error_code &error)
  const noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  unique_ref<CFDataRef> value = ::SecCertificateCopySerialNumberData(
    impl_.ref,
    nullptr
  );

  auto first = ::CFDataGetBytePtr(value.ref);
  auto last = first + ::CFDataGetLength(value.ref);

  while (first != last && !first[0])
  {
    ++first;
  }

  std::vector<uint8_t> result;
  try
  {
    result.assign(first, last);
    error.clear();
  }

  // LCOV_EXCL_START
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  // LCOV_EXCL_STOP

  return result;
}


std::vector<uint8_t> certificate_t::authority_key_identifier (
  std::error_code &error) const noexcept
{
  return key_identifier(impl_.ref, kSecOIDAuthorityKeyIdentifier, error);
}


std::vector<uint8_t> certificate_t::subject_key_identifier (
  std::error_code &error) const noexcept
{
  return key_identifier(impl_.ref, kSecOIDSubjectKeyIdentifier, error);
}


certificate_t::distinguished_name_t certificate_t::issuer (
  std::error_code &error) const noexcept
{
  return to_distinguished_name(impl_.ref, kSecOIDX509V1IssuerName, error);
}


certificate_t::distinguished_name_t certificate_t::issuer (const oid_t &oid,
  std::error_code &error) const noexcept
{
  return to_distinguished_name(impl_.ref, kSecOIDX509V1IssuerName, oid, error);
}


certificate_t::distinguished_name_t certificate_t::subject (
  std::error_code &error) const noexcept
{
  return to_distinguished_name(impl_.ref, kSecOIDX509V1SubjectName, error);
}


certificate_t::distinguished_name_t certificate_t::subject (const oid_t &oid,
  std::error_code &error) const noexcept
{
  return to_distinguished_name(impl_.ref, kSecOIDX509V1SubjectName, oid, error);
}


namespace {


std::string normalized_ip_string (CFTypeRef data)
{
  auto s = static_cast<CFStringRef>(data);

  char buf[INET6_ADDRSTRLEN];
  auto p = c_str(s, buf);

  if (::CFStringGetLength(s) == 39)
  {
    // convert to RFC5952 (recommended IPv6 textual representation) format
    in6_addr ip;
    net::ip::__bits::inet_pton(p, ip);
    return normalized_ip_string(reinterpret_cast<uint8_t *>(&ip), sizeof(ip));
  }

  return p;
}


std::vector<std::pair<certificate_t::alt_name, std::string>> to_alt_names (
  SecCertificateRef cert,
  CFTypeRef oid,
  std::error_code &error) noexcept
{
  static const auto
    dns_name = CFSTR("DNS Name"),
    ip_address = CFSTR("IP Address"),
    email_address = CFSTR("Email Address"),
    uri = CFSTR("URI");

  std::vector<std::pair<certificate_t::alt_name, std::string>> result;
  if (auto values = copy_values(cert, oid, error))
  {
    try
    {
      char buf[1024];
      for (auto i = 0U;  i != ::CFArrayGetCount(values.ref);  ++i)
      {
        auto entry = (CFDictionaryRef)::CFArrayGetValueAtIndex(values.ref, i);
        auto label = ::CFDictionaryGetValue(entry, kSecPropertyKeyLabel);
        if (::CFEqual(label, dns_name))
        {
          auto value = c_str(
            ::CFDictionaryGetValue(entry, kSecPropertyKeyValue), buf
          );
          result.emplace_back(certificate_t::alt_name::dns, value);
        }
        else if (::CFEqual(label, ip_address))
        {
          auto value = normalized_ip_string(
            ::CFDictionaryGetValue(entry, kSecPropertyKeyValue)
          );
          result.emplace_back(certificate_t::alt_name::ip, value);
        }
        else if (::CFEqual(label, uri))
        {
          auto value = c_str(
            ::CFURLGetString((CFURLRef)::CFDictionaryGetValue(entry, kSecPropertyKeyValue)), buf
          );
          result.emplace_back(certificate_t::alt_name::uri, value);
        }
        else if (::CFEqual(label, email_address))
        {
          auto value = c_str(
            ::CFDictionaryGetValue(entry, kSecPropertyKeyValue), buf
          );
          result.emplace_back(certificate_t::alt_name::email, value);
        }
      }
    }

    // LCOV_EXCL_START
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
      result.clear();
    }
    // LCOV_EXCL_STOP
  }
  return result;
}


//
// With MacOS SDK 10.14, SecCertificateCopyPublicKey was deprecated in favor
// of SecCertificateCopyKey. Use whichever is available
//


::SecKeyRef (*_SecCertificateCopyKey)(::SecCertificateRef) = nullptr;
::OSStatus (*_SecCertificateCopyPublicKey)(::SecCertificateRef, SecKeyRef *) = nullptr;

void init_cert_lib ()
{
  _SecCertificateCopyKey = (decltype(_SecCertificateCopyKey))dlsym(
    RTLD_DEFAULT,
    "SecCertificateCopyKey"
  );
  _SecCertificateCopyPublicKey = (decltype(_SecCertificateCopyPublicKey ))dlsym(
    RTLD_DEFAULT,
    "SecCertificateCopyPublicKey"
  );
}


} // namespace


std::vector<std::pair<certificate_t::alt_name, std::string>>
certificate_t::issuer_alt_names (std::error_code &error) const noexcept
{
  return to_alt_names(impl_.ref, kSecOIDIssuerAltName, error);
}


std::vector<std::pair<certificate_t::alt_name, std::string>>
certificate_t::subject_alt_names (std::error_code &error) const noexcept
{
  return to_alt_names(impl_.ref, kSecOIDSubjectAltName, error);
}


std::vector<uint8_t> certificate_t::apply (
  std::vector<uint8_t>(*fn)(const uint8_t *, size_t),
  std::error_code &error) const noexcept
{
  if (impl_.ref)
  {
    unique_ref<CFDataRef> der = ::SecCertificateCopyData(impl_.ref);
    return calculate_digest(
      ::CFDataGetBytePtr(der.ref),
      ::CFDataGetLength(der.ref),
      fn,
      error
    );
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


public_key_t certificate_t::public_key (std::error_code &error) const noexcept
{
  if (impl_.ref)
  {
    static std::once_flag once;
    std::call_once(once, &init_cert_lib);

    if (_SecCertificateCopyKey)
    {
      if (auto key = (*_SecCertificateCopyKey)(impl_.ref))
      {
        error.clear();
        return {key};
      }
    }
    else if (_SecCertificateCopyPublicKey)
    {
      __bits::public_key_t key;
      auto status = (*_SecCertificateCopyPublicKey)(impl_.ref, &key.ref);
      if (status == errSecSuccess)
      {
        return std::move(key);
      }
      error.assign(status, category());
      return {};
    }
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


namespace {


inline unique_ref<CFDictionaryRef> import_options (const std::string &passphrase)
  noexcept
{
  // kSecImportExportPassphrase
  auto import_passphrase = ::CFStringCreateWithBytesNoCopy(
    nullptr,
    reinterpret_cast<const uint8_t *>(&passphrase[0]),
    passphrase.size(),
    kCFStringEncodingUTF8,
    false,
    kCFAllocatorNull
  );

  // kSecImportExportAccess
  SecAccessRef access{};
  ::SecAccessCreate(CFSTR("Imported by SAL"), nullptr, &access);

  const void
    *keys[] =
    {
      kSecImportExportPassphrase,
      kSecImportExportAccess,
    },
    *values[] =
    {
      import_passphrase,
      access,
    };

  return ::CFDictionaryCreate(nullptr, keys, values, 2, nullptr, nullptr);
}


} // namespace


std::vector<certificate_t> certificate_t::import_pkcs12 (
  const uint8_t *first,
  const uint8_t *last,
  const std::string &passphrase,
  private_key_t *private_key,
  std::error_code &error) noexcept
{
  unique_ref<CFArrayRef> items = ::CFArrayCreate(nullptr, 0, 0, nullptr);
  auto status = ::SecPKCS12Import(
    data(first, last).ref,
    import_options(passphrase).ref,
    &items.ref
  );
  if (status != errSecSuccess)
  {
    error.assign(status, category());
    return {};
  }

  sal_assert(::CFArrayGetCount(items.ref) > 0);
  auto data = (CFDictionaryRef)::CFArrayGetValueAtIndex(items.ref, 0);

  auto chain = (CFArrayRef)::CFDictionaryGetValue(data, kSecImportItemCertChain);
  std::vector<certificate_t> certificates(::CFArrayGetCount(chain));
  for (auto i = 0U;  i < certificates.size();  ++i)
  {
    certificates[i].impl_.ref = (SecCertificateRef)::CFRetain(
      ::CFArrayGetValueAtIndex(chain, i)
    );
  }

  if (private_key)
  {
    auto identity = (SecIdentityRef)::CFDictionaryGetValue(data,
      kSecImportItemIdentity
    );
    __bits::private_key_t key;
    (void)::SecIdentityCopyPrivateKey(identity, &key.ref);
    *private_key = std::move(key);
  }

  error.clear();
  return certificates;
}


namespace {

inline auto keychain_certificates_query () noexcept
{
  static const void
    *keys[] =
    {
      kSecClass,
      kSecMatchTrustedOnly,
      kSecMatchLimit,
    },
    *values[] =
    {
      kSecClassCertificate,
      kCFBooleanTrue,
      kSecMatchLimitAll,
    };
  static auto query = ::CFDictionaryCreate(nullptr,
    keys,
    values,
    3,
    nullptr,
    nullptr
  );
  return query;
}

} // namespace


certificate_t certificate_t::load_first (
  std::function<bool(const certificate_t &)> predicate,
  std::error_code &error) noexcept
{
  error.clear();
  unique_ref<CFArrayRef> result;
  auto status = ::SecItemCopyMatching(keychain_certificates_query(),
    (CFTypeRef *)&result.ref
  );
  if (status == errSecSuccess)
  {
    for (auto i = 0;  i < ::CFArrayGetCount(result.ref);  ++i)
    {
      auto certificate = certificate_t::from_native_handle(
        (SecCertificateRef)::CFArrayGetValueAtIndex(result.ref, i)
      );
      ::CFRetain(certificate.native_handle().ref);
      if (predicate(certificate))
      {
        return certificate;
      }
    }
    error = std::make_error_code(std::errc::no_such_file_or_directory);
  }
  else
  {
    error.assign(status, category());
  }
  return {};
}


std::vector<certificate_t> certificate_t::load (
  std::function<bool(const certificate_t &)> predicate,
  std::error_code &error) noexcept
{
  error.clear();
  std::vector<certificate_t> certificates;
  try
  {
    unique_ref<CFArrayRef> result;
    auto status = ::SecItemCopyMatching(keychain_certificates_query(),
      (CFTypeRef *)&result.ref
    );
    if (status == errSecSuccess)
    {
      for (auto i = 0;  i < ::CFArrayGetCount(result.ref);  ++i)
      {
        auto certificate = certificate_t::from_native_handle(
          (SecCertificateRef)::CFArrayGetValueAtIndex(result.ref, i)
        );
        ::CFRetain(certificate.native_handle().ref);
        if (predicate(certificate))
        {
          certificates.emplace_back(std::move(certificate));
        }
      }
    }
    else
    {
      error.assign(status, category());
    }
  }
  catch (const std::bad_alloc &)
  {
    certificates.clear();
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  return certificates;
}


#elif __sal_os_linux //{{{1


bool certificate_t::issued_by (const certificate_t &issuer,
  std::error_code &error) const noexcept
{
  if (impl_ && issuer.impl_)
  {
    error.clear();
    return X509_check_issued(issuer.impl_.ref, impl_.ref) == X509_V_OK;
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


certificate_t certificate_t::from_der (const uint8_t *first, const uint8_t *last,
  std::error_code &error) noexcept
{
  auto cert = d2i_X509(nullptr, &first, (last - first));
  if (cert)
  {
    error.clear();
  }
  else
  {
    error = std::make_error_code(std::errc::invalid_argument);
  }
  return __bits::certificate_t{cert};
}


bool certificate_t::operator== (const certificate_t &that) const noexcept
{
  if (impl_.ref && that.impl_.ref)
  {
    return X509_cmp(impl_.ref, that.impl_.ref) == 0;
  }
  else if (!impl_.ref && !that.impl_.ref)
  {
    return true;
  }
  return false;
}


uint8_t *certificate_t::to_der (uint8_t *first, uint8_t *last,
  std::error_code &error) const noexcept
{
  if (!impl_.ref)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  auto size = i2d_X509(impl_.ref, nullptr);
  if (size > last - first)
  {
    error = std::make_error_code(std::errc::result_out_of_range);
    return {};
  }

  error.clear();

  i2d_X509(impl_.ref, &first);
  return first;
}


std::vector<uint8_t> certificate_t::to_der (std::error_code &error)
  const noexcept
{
  if (!impl_.ref)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  try
  {
    auto size = i2d_X509(impl_.ref, nullptr);
    std::vector<uint8_t> result(size);
    auto begin = &result[0];
    i2d_X509(impl_.ref, &begin);
    return result;
  }

  // LCOV_EXCL_START
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return {};
  }
  // LCOV_EXCL_STOP
}


int certificate_t::version () const noexcept
{
  return impl_.ref ? X509_get_version(impl_.ref) + 1 : 0;
}


namespace {


sal::time_t to_time (const ASN1_TIME *time, std::error_code &error) noexcept
{
  if (ASN1_TIME_check(const_cast<ASN1_TIME *>(time)) == 0)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return {};
  }

  std::tm tm{};
  auto in = static_cast<const unsigned char *>(time->data);

  if (time->type == V_ASN1_UTCTIME)
  {
    // two-digit year
    tm.tm_year = (in[0] - '0') * 10 + (in[1] - '0');
    in += 2;

    if (tm.tm_year < 70)
    {
      tm.tm_year += 100;
    }
  }
  else if (time->type == V_ASN1_GENERALIZEDTIME)
  {
    // four-digit year
    tm.tm_year =
      (in[0] - '0') * 1000 +
      (in[1] - '0') * 100 +
      (in[2] - '0') * 10 +
      (in[3] - '0');
    in += 4;

    tm.tm_year -= 1900;
  }

  tm.tm_mon = (in[0] - '0') * 10 + (in[1] - '0') - 1;
  in += 2;

  tm.tm_mday = (in[0] - '0') * 10 + (in[1] - '0');
  in += 2;

  tm.tm_hour = (in[0] - '0') * 10 + (in[1] - '0');
  in += 2;

  tm.tm_min = (in[0] - '0') * 10 + (in[1] - '0');
  in += 2;

  tm.tm_sec = (in[0] - '0') * 10 + (in[1] - '0');

  // ignoring fractional seconds and timezones

  return sal::clock_t::from_time_t(mktime(&tm));
}


} // namespace


sal::time_t certificate_t::not_before (std::error_code &error) const noexcept
{
  if (impl_)
  {
    error.clear();
    return to_time(X509_get_notBefore(impl_.ref), error);
  }
  else
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }
}


sal::time_t certificate_t::not_after (std::error_code &error) const noexcept
{
  if (impl_)
  {
    error.clear();
    return to_time(X509_get_notAfter(impl_.ref), error);
  }
  else
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }
}


std::vector<uint8_t> certificate_t::serial_number (std::error_code &error)
  const noexcept
{
  std::vector<uint8_t> result;

  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return result;
  }

  if (auto bn = ASN1_INTEGER_to_BN(X509_get_serialNumber(impl_.ref), nullptr))
  {
    try
    {
      result.resize(BN_num_bytes(bn));
      BN_bn2bin(bn, &result[0]);
      error.clear();
    }

    // LCOV_EXCL_START
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
    }
    // LCOV_EXCL_STOP

    BN_free(bn);
  }

  // LCOV_EXCL_START
  else
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  // LCOV_EXCL_STOP

  return result;
}


std::vector<uint8_t> certificate_t::authority_key_identifier (
  std::error_code &error) const noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  auto index = X509_get_ext_by_NID(impl_.ref, NID_authority_key_identifier, -1);
  if (index < 0)
  {
    error.clear();
    return {};
  }

  auto decoded = static_cast<AUTHORITY_KEYID *>(
    X509V3_EXT_d2i(X509_get_ext(impl_.ref, index))
  );

  std::vector<uint8_t> result;
  try
  {
    result.assign(
      decoded->keyid->data,
      decoded->keyid->data + decoded->keyid->length
    );
    error.clear();
  }

  // LCOV_EXCL_START
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  // LCOV_EXCL_STOP

  AUTHORITY_KEYID_free(decoded);

  return result;
}


std::vector<uint8_t> certificate_t::subject_key_identifier (
  std::error_code &error) const noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  auto index = X509_get_ext_by_NID(impl_.ref, NID_subject_key_identifier, -1);
  if (index < 0)
  {
    error.clear();
    return {};
  }

  auto decoded = static_cast<ASN1_OCTET_STRING *>(
    X509V3_EXT_d2i(X509_get_ext(impl_.ref, index))
  );

  std::vector<uint8_t> result;
  try
  {
    result.assign(decoded->data, decoded->data + decoded->length);
    error.clear();
  }

  // LCOV_EXCL_START
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  // LCOV_EXCL_STOP

  ASN1_OCTET_STRING_free(decoded);

  return result;
}


namespace {


inline std::string to_string (const ASN1_STRING *s) noexcept
{
#if OPENSSL_VERSION_NUMBER < 0x10100000
  return reinterpret_cast<const char *>(
    ASN1_STRING_data(const_cast<ASN1_STRING *>(s))
  );
#else
  return reinterpret_cast<const char *>(ASN1_STRING_get0_data(s));
#endif
}


auto to_distinguished_name (X509_NAME *name, std::error_code &error) noexcept
{
  certificate_t::distinguished_name_t result;

  try
  {
    for (auto i = 0;  i != X509_NAME_entry_count(name);  ++i)
    {
      auto entry = X509_NAME_get_entry(name, i);

      char oid[128];
      OBJ_obj2txt(oid, sizeof(oid), X509_NAME_ENTRY_get_object(entry), 1);

      result.emplace_back(oid, to_string(X509_NAME_ENTRY_get_data(entry)));
    }
    error.clear();
  }

  // LCOV_EXCL_START
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    result.clear();
  }
  // LCOV_EXCL_STOP

  return result;
}


auto to_distinguished_name (X509_NAME *name, const oid_t &filter_oid,
  std::error_code &error) noexcept
{
  certificate_t::distinguished_name_t result;

  auto filter_nid = OBJ_txt2nid(filter_oid.c_str());
  if (filter_nid == NID_undef)
  {
    error.clear();
    return result;
  }

  try
  {
    for (auto i = 0;  i != X509_NAME_entry_count(name);  ++i)
    {
      auto entry = X509_NAME_get_entry(name, i);
      if (OBJ_obj2nid(X509_NAME_ENTRY_get_object(entry)) == filter_nid)
      {
        result.emplace_back(filter_oid, to_string(X509_NAME_ENTRY_get_data(entry)));
      }
    }
    error.clear();
  }

  // LCOV_EXCL_START
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    result.clear();
  }
  // LCOV_EXCL_STOP

  return result;
}


} // namespace


certificate_t::distinguished_name_t certificate_t::issuer (
  std::error_code &error) const noexcept
{
  if (impl_)
  {
    return to_distinguished_name(X509_get_issuer_name(impl_.ref), error);
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


certificate_t::distinguished_name_t certificate_t::issuer (const oid_t &oid,
  std::error_code &error) const noexcept
{
  if (impl_)
  {
    return to_distinguished_name(X509_get_issuer_name(impl_.ref), oid, error);
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


certificate_t::distinguished_name_t certificate_t::subject (
  std::error_code &error) const noexcept
{
  if (impl_)
  {
    return to_distinguished_name(X509_get_subject_name(impl_.ref), error);
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


certificate_t::distinguished_name_t certificate_t::subject (const oid_t &oid,
  std::error_code &error) const noexcept
{
  if (impl_)
  {
    return to_distinguished_name(X509_get_subject_name(impl_.ref), oid, error);
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


namespace {

std::vector<std::pair<certificate_t::alt_name, std::string>> to_alt_names (
  X509 *cert, int nid, std::error_code &error) noexcept
{
  std::vector<std::pair<certificate_t::alt_name, std::string>> result;

  if (!cert)
  {
    error = std::make_error_code(std::errc::bad_address);
    return result;
  }

  auto names = static_cast<STACK_OF(GENERAL_NAME) *>(
    X509_get_ext_d2i(cert, nid, nullptr, nullptr)
  );
  if (!names)
  {
    return result;
  }

  try
  {
    for (auto i = 0;  i != sk_GENERAL_NAME_num(names);  ++i)
    {
      auto name = sk_GENERAL_NAME_value(names, i);
      switch (name->type)
      {
        case GEN_EMAIL:
        {
          auto s = name->d.rfc822Name;
          if (s && s->type == V_ASN1_IA5STRING && s->data && s->length > 0)
          {
            auto v = std::string(s->data, s->data + s->length);
            result.emplace_back(certificate_t::alt_name::email, std::move(v));
          }
          break;
        }

        case GEN_DNS:
        {
          auto s = name->d.dNSName;
          if (s && s->type == V_ASN1_IA5STRING && s->data && s->length > 0)
          {
            auto v = std::string(s->data, s->data + s->length);
            result.emplace_back(certificate_t::alt_name::dns, std::move(v));
          }
          break;
        }

        case GEN_URI:
        {
          auto s = name->d.uniformResourceIdentifier;
          if (s && s->type == V_ASN1_IA5STRING && s->data && s->length > 0)
          {
            auto v = std::string(s->data, s->data + s->length);
            result.emplace_back(certificate_t::alt_name::uri, std::move(v));
          }
          break;
        }

        case GEN_IPADD:
        {
          auto s = name->d.iPAddress;
          if (s && s->type == V_ASN1_OCTET_STRING && s->data
            && (s->length == 4 || s->length == 16))
          {
            auto v = normalized_ip_string(s->data, s->length);
            result.emplace_back(certificate_t::alt_name::ip, std::move(v));
          }
          break;
        }
      }
    }
  }

  // LCOV_EXCL_START
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    result.clear();
  }
  // LCOV_EXCL_STOP

  sk_GENERAL_NAME_pop_free(names, GENERAL_NAME_free);

  return result;
}

} // namespace


std::vector<std::pair<certificate_t::alt_name, std::string>>
certificate_t::issuer_alt_names (std::error_code &error) const noexcept
{
  return to_alt_names(impl_.ref, NID_issuer_alt_name, error);
}


std::vector<std::pair<certificate_t::alt_name, std::string>>
certificate_t::subject_alt_names (std::error_code &error) const noexcept
{
  return to_alt_names(impl_.ref, NID_subject_alt_name, error);
}


std::vector<uint8_t> certificate_t::apply (
  std::vector<uint8_t>(*fn)(const uint8_t *, size_t),
  std::error_code &error) const noexcept
{
  if (impl_.ref)
  {
    uint8_t der[16 * 1024];
    if (auto end = to_der(der, der + sizeof(der), error))
    {
      return calculate_digest(der, (end - der), fn, error);
    }
  }
  else
  {
    error = std::make_error_code(std::errc::bad_address);
  }
  return {};
}


public_key_t certificate_t::public_key (std::error_code &error) const noexcept
{
  if (impl_.ref)
  {
    return __bits::public_key_t(X509_get_pubkey(impl_.ref));
  }
  else
  {
    error = std::make_error_code(std::errc::bad_address);
  }
  return {};
}


namespace {

using pkcs12_ptr = unique_ref<PKCS12 *, PKCS12_free>;

inline void init_openssl () noexcept
{
  static std::once_flag init_flag;
  std::call_once(init_flag,
    []()
    {
      OpenSSL_add_all_algorithms();
    }
  );
}

} // namespace


std::vector<certificate_t> certificate_t::import_pkcs12 (
  const uint8_t *first, const uint8_t *last,
  const std::string &passphrase,
  private_key_t *private_key,
  std::error_code &error) noexcept
{
  init_openssl();

  pkcs12_ptr p12 = d2i_PKCS12(nullptr, &first, last - first);
  if (!p12)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return {};
  }

  __bits::certificate_t certificate;
  __bits::private_key_t pkey;
  STACK_OF(X509) *chain = nullptr;

  auto result = PKCS12_parse(p12.ref,
    passphrase.c_str(),
    &pkey.ref,
    &certificate.ref,
    &chain
  );
  if (!result)
  {
    error.assign(ERR_get_error(), category());
    return {};
  }

  std::vector<certificate_t> certificates;
  certificates.reserve(sk_X509_num(chain) + 1);

  certificates.push_back(std::move(certificate));
  while (auto x509 = sk_X509_pop(chain))
  {
    certificates.push_back(__bits::certificate_t(x509));
  }

  if (private_key)
  {
    *private_key = std::move(pkey);
  }

  error.clear();
  return certificates;
}


namespace {


inline const char *default_ca_file () noexcept
{
  if (auto path = std::getenv(X509_get_default_cert_file_env()))
  {
    return path;
  }
  return X509_get_default_cert_file();
}


const char *ca_file () noexcept
{
  // see https://www.happyassassin.net/2015/01/12/a-note-about-ssltls-trusted-certificate-stores-and-platforms/
  static const char *files[] =
  {
    default_ca_file(),
    "/etc/pki/tls/certs/ca-bundle.crt",
    "/etc/ssl/certs/ca-certificates.crt",
  };
  for (auto &file: files)
  {
    struct stat st;
    if (stat(file, &st) == 0 && S_ISREG(st.st_mode))
    {
      return file;
    }
  }
  return nullptr;
}


auto ca_file_bio (std::error_code &error) noexcept
{
  auto bio = std::unique_ptr<BIO, decltype(&BIO_free_all)>(nullptr, &BIO_free_all);

  static auto file = ca_file();
  if (!file)
  {
    error = std::make_error_code(std::errc::no_such_file_or_directory);
    return bio;
  }

  bio.reset(BIO_new(BIO_s_file()));
  if (BIO_read_filename(bio.get(), ca_file()) < 1)
  {
    error.assign(ERR_get_error(), category());
    bio.reset();
  }

  return bio;
}


} // namespace


certificate_t certificate_t::load_first (
  std::function<bool(const certificate_t &)> predicate,
  std::error_code &error) noexcept
{
  auto bio = ca_file_bio(error);
  if (!bio)
  {
    return {};
  }

  auto certificates = PEM_X509_INFO_read_bio(bio.get(),
    nullptr,
    nullptr,
    nullptr
  );

  for (auto i = 0;  i < sk_X509_INFO_num(certificates);  ++i)
  {
    auto certificate = from_native_handle(
      sk_X509_INFO_value(certificates, i)->x509
    );
    __bits::inc_ref(certificate.native_handle().ref);
    if (predicate(certificate))
    {
      sk_X509_INFO_pop_free(certificates, X509_INFO_free);
      error.clear();
      return certificate;
    }
  }

  sk_X509_INFO_pop_free(certificates, X509_INFO_free);
  error = std::make_error_code(std::errc::no_such_file_or_directory);
  return {};
}


std::vector<certificate_t> certificate_t::load (
  std::function<bool(const certificate_t &)> predicate,
  std::error_code &error) noexcept
{
  auto bio = ca_file_bio(error);
  if (!bio)
  {
    return {};
  }

  std::vector<certificate_t> result;
  auto certificates = PEM_X509_INFO_read_bio(bio.get(),
    nullptr,
    nullptr,
    nullptr
  );

  try
  {
    for (auto i = 0;  i < sk_X509_INFO_num(certificates);  ++i)
    {
      auto certificate = from_native_handle(
        sk_X509_INFO_value(certificates, i)->x509
      );
      __bits::inc_ref(certificate.native_handle().ref);
      if (predicate(certificate))
      {
        result.emplace_back(std::move(certificate));
      }
    }
    error.clear();
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    result.clear();
  }

  sk_X509_INFO_pop_free(certificates, X509_INFO_free);

  return result;
}


#elif __sal_os_windows // {{{1


bool certificate_t::issued_by (const certificate_t &issuer,
  std::error_code &error) const noexcept
{
  if (impl_ && issuer.impl_)
  {
    error.clear();
    return ::CertCompareCertificateName(
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
      &issuer.impl_.ref->pCertInfo->Subject,
      &impl_.ref->pCertInfo->Issuer
    );
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


certificate_t certificate_t::from_der (const uint8_t *first, const uint8_t *last,
  std::error_code &error) noexcept
{
  auto cert = ::CertCreateCertificateContext(
    X509_ASN_ENCODING,
    first,
    static_cast<DWORD>(last - first)
  );
  if (cert)
  {
    error.clear();
  }
  else
  {
    error = std::make_error_code(std::errc::invalid_argument);
  }
  return __bits::certificate_t{cert};
}


bool certificate_t::operator== (const certificate_t &that) const noexcept
{
  if (impl_.ref && that.impl_.ref)
  {
    return ::CertCompareCertificate(
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
      impl_.ref->pCertInfo,
      that.impl_.ref->pCertInfo
    ) != 0;
  }
  else if (!impl_.ref && !that.impl_.ref)
  {
    return true;
  }
  return false;
}


uint8_t *certificate_t::to_der (uint8_t *first, uint8_t *last,
  std::error_code &error) const noexcept
{
  if (impl_.ref)
  {
    if (last - first >= impl_.ref->cbCertEncoded)
    {
      error.clear();
      memcpy(first, impl_.ref->pbCertEncoded, impl_.ref->cbCertEncoded);
      return first + impl_.ref->cbCertEncoded;
    }
    error = std::make_error_code(std::errc::result_out_of_range);
  }
  else
  {
    error = std::make_error_code(std::errc::bad_address);
  }
  return {};
}


std::vector<uint8_t> certificate_t::to_der (std::error_code &error)
  const noexcept
{
  if (impl_.ref)
  {
    try
    {
      error.clear();
      return std::vector<uint8_t>(
        impl_.ref->pbCertEncoded,
        impl_.ref->pbCertEncoded + impl_.ref->cbCertEncoded
      );
    }
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
    }
  }
  else
  {
    error = std::make_error_code(std::errc::bad_address);
  }
  return {};
}


int certificate_t::version () const noexcept
{
  return impl_.ref ? impl_.ref->pCertInfo->dwVersion + 1 : 0;
}


namespace {

sal::time_t to_time (const FILETIME &time, std::error_code &error) noexcept
{
  SYSTEMTIME sys_time;
  if (::FileTimeToSystemTime(&time, &sys_time))
  {
    std::tm tm{};
    tm.tm_sec = sys_time.wSecond;
    tm.tm_min = sys_time.wMinute;
    tm.tm_hour = sys_time.wHour;
    tm.tm_mday = sys_time.wDay;
    tm.tm_mon = sys_time.wMonth - 1;
    tm.tm_year = sys_time.wYear - 1900;

    error.clear();
    return sal::clock_t::from_time_t(mktime(&tm));
  }

  error = std::make_error_code(std::errc::invalid_argument);
  return {};
}

} // namespace


sal::time_t certificate_t::not_before (std::error_code &error) const noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  return to_time(impl_.ref->pCertInfo->NotBefore, error);
}


sal::time_t certificate_t::not_after (std::error_code &error) const noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  return to_time(impl_.ref->pCertInfo->NotAfter, error);
}


std::vector<uint8_t> certificate_t::serial_number (std::error_code &error)
  const noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  try
  {
    auto first = impl_.ref->pCertInfo->SerialNumber.pbData;
    auto last = first + impl_.ref->pCertInfo->SerialNumber.cbData;

    while (last != first && !last[-1])
    {
      --last;
    }

    std::vector<uint8_t> result;
    while (first != last)
    {
      result.push_back(*--last);
    }

    error.clear();
    return result;
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return {};
  }
}


std::vector<uint8_t> certificate_t::authority_key_identifier (
  std::error_code &error) const noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  auto ext = ::CertFindExtension(szOID_AUTHORITY_KEY_IDENTIFIER2,
    impl_.ref->pCertInfo->cExtension,
    impl_.ref->pCertInfo->rgExtension
  );
  if (!ext)
  {
    error.clear();
    return {};
  }

  CERT_AUTHORITY_KEY_ID2_INFO *decoded;
  DWORD length = 0;
  ::CryptDecodeObjectEx(X509_ASN_ENCODING,
    X509_AUTHORITY_KEY_ID2,
    ext->Value.pbData,
    ext->Value.cbData,
    CRYPT_DECODE_ALLOC_FLAG,
    0,
    &decoded,
    &length
  );
  if (!decoded)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return {};
  }

  std::vector<uint8_t> result;
  try
  {
    result.assign(
      decoded->KeyId.pbData,
      decoded->KeyId.pbData + decoded->KeyId.cbData
    );
    error.clear();
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  ::LocalFree(decoded);

  return result;
}


std::vector<uint8_t> certificate_t::subject_key_identifier (
  std::error_code &error) const noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  auto ext = ::CertFindExtension(szOID_SUBJECT_KEY_IDENTIFIER,
    impl_.ref->pCertInfo->cExtension,
    impl_.ref->pCertInfo->rgExtension
  );
  if (!ext)
  {
    error.clear();
    return {};
  }

  CRYPT_DATA_BLOB *decoded;
  DWORD length = 0;
  ::CryptDecodeObjectEx(X509_ASN_ENCODING,
    szOID_SUBJECT_KEY_IDENTIFIER,
    ext->Value.pbData,
    ext->Value.cbData,
    CRYPT_DECODE_ALLOC_FLAG,
    0,
    &decoded,
    &length
  );
  if (!decoded)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return {};
  }

  std::vector<uint8_t> result;
  try
  {
    result.assign(
      decoded->pbData,
      decoded->pbData + decoded->cbData
    );
    error.clear();
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  ::LocalFree(decoded);

  return result;
}


namespace {


auto encoded_name_list (const CERT_NAME_BLOB &name, std::error_code &error)
  noexcept
{
  constexpr auto decode_flags =
    CRYPT_DECODE_ALLOC_FLAG |
    CRYPT_DECODE_NOCOPY_FLAG |
    CRYPT_DECODE_SHARE_OID_STRING_FLAG;

  HLOCAL rdn_buf{};
  DWORD rdn_size{};

  auto result = ::CryptDecodeObjectEx(
    X509_ASN_ENCODING,
    X509_NAME,
    name.pbData,
    name.cbData,
    decode_flags,
    nullptr,
    &rdn_buf,
    &rdn_size
  );

  if (!result)
  {
    error.assign(::GetLastError(), std::system_category());
  }

  return rdn_buf;
}


auto to_distinguished_name (const CERT_NAME_BLOB &name,
  const oid_t *filter_oid, std::error_code &error) noexcept
{
  certificate_t::distinguished_name_t result;

  auto rdn_buf = encoded_name_list(name, error);
  if (!rdn_buf)
  {
    return result;
  }

  try
  {
    auto &rdn = *reinterpret_cast<CERT_NAME_INFO *>(rdn_buf);
    for (auto i = 0U;  i != rdn.cRDN;  ++i)
    {
      for (auto j = 0U;  j != rdn.rgRDN[i].cRDNAttr;  ++j)
      {
        auto &rdn_attr = rdn.rgRDN[i].rgRDNAttr[j];
        if (filter_oid && *filter_oid != rdn_attr.pszObjId)
        {
          continue;
        }

        char value_string[1024];
        CertRDNValueToStr(rdn_attr.dwValueType,
          &rdn_attr.Value,
          value_string,
          sizeof(value_string)
        );

        result.emplace_back(rdn_attr.pszObjId, value_string);
      }
    }
    error.clear();
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    result.clear();
  }

  ::LocalFree(rdn_buf);

  return result;
}


} // namespace


certificate_t::distinguished_name_t certificate_t::issuer (
  std::error_code &error) const noexcept
{
  if (impl_)
  {
    return to_distinguished_name(impl_.ref->pCertInfo->Issuer, nullptr, error);
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


certificate_t::distinguished_name_t certificate_t::issuer (const oid_t &oid,
  std::error_code &error) const noexcept
{
  if (impl_)
  {
    return to_distinguished_name(impl_.ref->pCertInfo->Issuer, &oid, error);
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


certificate_t::distinguished_name_t certificate_t::subject (
  std::error_code &error) const noexcept
{
  if (impl_)
  {
    return to_distinguished_name(impl_.ref->pCertInfo->Subject, nullptr, error);
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


certificate_t::distinguished_name_t certificate_t::subject (const oid_t &oid,
  std::error_code &error) const noexcept
{
  if (impl_)
  {
    return to_distinguished_name(impl_.ref->pCertInfo->Subject, &oid, error);
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


namespace {


inline std::string to_string (LPWSTR in)
{
  char out[256];
  auto size = ::WideCharToMultiByte(
    CP_ACP,
    0,
    in, -1,
    out, sizeof(out),
    nullptr,
    nullptr
  );
  return std::string(out, out + size - 1);
}


std::vector<std::pair<certificate_t::alt_name, std::string>> to_alt_names (
  PCCERT_CONTEXT cert,
  LPCSTR oid,
  std::error_code &error) noexcept
{
  std::vector<std::pair<certificate_t::alt_name, std::string>> result;

  if (!cert)
  {
    error = std::make_error_code(std::errc::bad_address);
    return result;
  }

  auto extension = ::CertFindExtension(oid,
    cert->pCertInfo->cExtension,
    cert->pCertInfo->rgExtension
  );
  if (!extension)
  {
    return result;
  }

  CERT_ALT_NAME_INFO *alt_name;
  DWORD size = 0;
  auto status = ::CryptDecodeObjectEx(
    X509_ASN_ENCODING,
    X509_ALTERNATE_NAME,
    extension->Value.pbData,
    extension->Value.cbData,
    CRYPT_DECODE_ALLOC_FLAG,
    nullptr,
    &alt_name,
    &size
  );
  if (!status)
  {
    error.assign(::GetLastError(), std::system_category());
    return result;
  }

  try
  {
    for (auto i = 0U;  i != alt_name->cAltEntry;  ++i)
    {
      auto &entry = alt_name->rgAltEntry[i];
      switch (entry.dwAltNameChoice)
      {
        case CERT_ALT_NAME_RFC822_NAME:
          result.emplace_back(certificate_t::alt_name::email,
            to_string(entry.pwszRfc822Name)
          );
          break;

        case CERT_ALT_NAME_DNS_NAME:
          result.emplace_back(certificate_t::alt_name::dns,
            to_string(entry.pwszDNSName)
          );
          break;

        case CERT_ALT_NAME_URL:
          result.emplace_back(certificate_t::alt_name::uri,
            to_string(entry.pwszURL)
          );
          break;

        case CERT_ALT_NAME_IP_ADDRESS:
          result.emplace_back(certificate_t::alt_name::ip,
            normalized_ip_string(entry.IPAddress.pbData, entry.IPAddress.cbData)
          );
          break;
      }
    }
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    result.clear();
  }

  LocalFree(alt_name);

  return result;
}


} // namespace


std::vector<std::pair<certificate_t::alt_name, std::string>>
certificate_t::issuer_alt_names (std::error_code &error) const noexcept
{
  return to_alt_names(impl_.ref, szOID_ISSUER_ALT_NAME2, error);
}


std::vector<std::pair<certificate_t::alt_name, std::string>>
certificate_t::subject_alt_names (std::error_code &error) const noexcept
{
  return to_alt_names(impl_.ref, szOID_SUBJECT_ALT_NAME2, error);
}


std::vector<uint8_t> certificate_t::apply (
  std::vector<uint8_t>(*fn)(const uint8_t *, size_t),
  std::error_code &error) const noexcept
{
  if (impl_.ref)
  {
    return calculate_digest(
      impl_.ref->pbCertEncoded,
      impl_.ref->cbCertEncoded,
      fn,
      error
    );
  }
  error = std::make_error_code(std::errc::bad_address);
  return {};
}


public_key_t certificate_t::public_key (std::error_code &error) const noexcept
{
  if (impl_.ref)
  {
    __bits::public_key_t key;
    auto status = ::CryptImportPublicKeyInfoEx2(
      X509_ASN_ENCODING,
      &impl_.ref->pCertInfo->SubjectPublicKeyInfo,
      0,
      nullptr,
      &key.ref
    );
    if (status)
    {
      return key;
    }
    error.assign(::GetLastError(), std::system_category());
  }
  else
  {
    error = std::make_error_code(std::errc::bad_address);
  }
  return {};
}


namespace {


template <size_t N>
inline wchar_t *to_wide (const std::string &v, wchar_t (&buf)[N]) noexcept
{
  ::MultiByteToWideChar(
    CP_UTF8,
    0,
    v.c_str(),
    -1,
    buf,
    N - 1
  );
  buf[N - 1] = L'\0';

  return buf;
}


} // namespace


std::vector<certificate_t> certificate_t::import_pkcs12 (
  const uint8_t *first, const uint8_t *last,
  const std::string &passphrase,
  private_key_t *private_key,
  std::error_code &error) noexcept
{
  CRYPT_DATA_BLOB pfx;
  pfx.pbData = const_cast<uint8_t *>(first);
  pfx.cbData = static_cast<DWORD>(last - first);

  wchar_t pwd[1024];
  auto store = ::PFXImportCertStore(&pfx, to_wide(passphrase, pwd), 0);
  ::SecureZeroMemory(pwd, sizeof(pwd));

  if (!store)
  {
    error.assign(::GetLastError(), category());
    return {};
  }

  std::vector<certificate_t> certificates;
  PCCERT_CONTEXT it = nullptr;
  while ((it = ::CertEnumCertificatesInStore(store, it)) != nullptr)
  {
    certificates.push_back(
      __bits::certificate_t(CertDuplicateCertificateContext(it))
    );
  }
  ::CertCloseStore(store, 0);
  std::reverse(certificates.begin(), certificates.end());

  if (!certificates.empty() && private_key)
  {
    DWORD pkey_spec;
    HCRYPTPROV_OR_NCRYPT_KEY_HANDLE pkey_handle;
    BOOL pkey_owner;

    auto status = ::CryptAcquireCertificatePrivateKey(
      certificates[0].impl_.ref,
      CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG,
      nullptr,
      &pkey_handle,
      &pkey_spec,
      &pkey_owner
    );

    if (status)
    {
      if (pkey_owner && (pkey_spec & CERT_NCRYPT_KEY_SPEC))
      {
        __bits::private_key_t key = pkey_handle;
        *private_key = std::move(key);
      }
      // else: not owner or not CNG private key -- do not take ownership
    }
    else
    {
      error.assign(::GetLastError(), std::system_category());
      return {};
    }
  }

  error.clear();
  return certificates;
}


namespace {

inline void close_store (HCERTSTORE ref) noexcept
{
  (void)::CertCloseStore(ref, 0);
}

using store_t = unique_ref<HCERTSTORE, close_store>;


constexpr LPCSTR subsystems[] =
{
  "MY",
  "Root",
  "Trust",
  "CA",
};


store_t current_user_store (LPCSTR subsystem, std::error_code &error)
  noexcept
{
  if (!error)
  {
    if (store_t store = ::CertOpenSystemStore(NULL, subsystem))
    {
      return store;
    }
    error.assign(::GetLastError(), category());
  }
  return {};
}


void for_each (store_t store,
  std::vector<certificate_t> &gathered,
  const std::function<bool(const certificate_t &)> &predicate)
{
  if (store)
  {
    PCCERT_CONTEXT it = nullptr;
    while ((it = ::CertEnumCertificatesInStore(store.ref, it)) != nullptr)
    {
      auto certificate = certificate_t::from_native_handle(
        ::CertDuplicateCertificateContext(it)
      );
      if (predicate(certificate))
      {
        gathered.emplace_back(std::move(certificate));
      }
    }
  }
}


certificate_t until_first (store_t store,
  const std::function<bool(const certificate_t &)> &predicate)
{
  if (store)
  {
    PCCERT_CONTEXT it = nullptr;
    while ((it = ::CertEnumCertificatesInStore(store.ref, it)) != nullptr)
    {
      auto certificate = certificate_t::from_native_handle(
        ::CertDuplicateCertificateContext(it)
      );
      if (predicate(certificate))
      {
        return certificate;
      }
    }
  }
  return {};
}


} // namespace


certificate_t certificate_t::load_first (
  std::function<bool(const certificate_t &)> predicate,
  std::error_code &error) noexcept
{
  error.clear();
  for (auto &subsystem: subsystems)
  {
    if (auto result = until_first(current_user_store(subsystem, error), predicate))
    {
      return result;
    }
  }
  error = std::make_error_code(std::errc::no_such_file_or_directory);
  return {};
}


std::vector<certificate_t> certificate_t::load (
  std::function<bool(const certificate_t &)> predicate,
  std::error_code &error) noexcept
{
  try
  {
    error.clear();
    std::vector<certificate_t> result;
    for (auto &subsystem: subsystems)
    {
      for_each(current_user_store(subsystem, error), result, predicate);
    }
    return result;
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
  return {};
}


#endif // }}}1


} // namespace crypto


__sal_end
