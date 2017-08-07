#include <sal/crypto/certificate.hpp>
#include <sal/net/ip/__bits/inet.hpp>
#include <sal/encode.hpp>
#include <sstream>


#if __sal_os_darwin //{{{1
  #if !defined(__apple_build_version__)
    #define availability(...) /**/
  #endif
  #include <CoreFoundation/CFNumber.h>
  #include <CoreFoundation/CFURL.h>
  #include <Security/SecCertificateOIDs.h>
#elif __sal_os_linux //{{{1
  #include <openssl/asn1.h>
  #include <openssl/x509v3.h>
#endif //}}}1


__sal_begin


namespace crypto {


namespace {


std::string unwrap (const std::string &pem)
{
  static constexpr char
    BEGIN[] = "-----BEGIN",
    END[] = "-----END";

  auto envelope_begin = pem.find(BEGIN, 0, sizeof(BEGIN) - 1);
  if (envelope_begin == pem.npos)
  {
    // not armored at all
    return pem;
  }

  // extract data since BEGIN
  auto line = pem.substr(envelope_begin, pem.npos);
  std::istringstream iss{line};

  // ignore BEGIN and read until END
  iss.ignore(pem.size(), '\n');
  std::string result;
  while (std::getline(iss, line))
  {
    if (line.find(END, 0, sizeof(END) - 1) == 0)
    {
      break;
    }
    result += line;
  }

  return result;
}


uint8_t *pem_to_der (const std::string &pem, uint8_t *buf, size_t buf_size,
  std::error_code &error) noexcept
{
  try
  {
    auto unwrapped_pem = unwrap(pem);

    auto decoded_size = max_decoded_size<base64>(unwrapped_pem, error);
    if (error)
    {
      return nullptr;
    }

    if (decoded_size > buf_size)
    {
      error = std::make_error_code(std::errc::no_buffer_space);
      return nullptr;
    }

    return decode<base64>(unwrapped_pem, buf, error);
  }

  // LCOV_EXCL_START
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return {};
  }
  // LCOV_EXCL_STOP
}


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


} // namespace


certificate_t certificate_t::from_pem (const std::string &data,
  std::error_code &error) noexcept
{
  uint8_t der[8192];
  if (auto end = pem_to_der(data, der, sizeof(der), error))
  {
    return certificate_t(der, end, error);
  }
  return certificate_t{};
}


#if __sal_os_darwin // {{{1


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


scoped_ref<CFArrayRef> copy_values (SecCertificateRef cert, CFTypeRef oid,
  std::error_code &error) noexcept
{
  if (cert)
  {
    scoped_ref<CFArrayRef> keys = ::CFArrayCreate(nullptr,
      &oid, 1,
      &kCFTypeArrayCallBacks
    );
    scoped_ref<CFDictionaryRef> dir = ::SecCertificateCopyValues(cert,
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
      scoped_ref<CFStringRef> filter = ::CFStringCreateWithBytesNoCopy(
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


} // namespace


certificate_t::certificate_t (const uint8_t *first, const uint8_t *last,
  std::error_code &error) noexcept
{
  if (last <= first)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return;
  }

  scoped_ref<CFDataRef> data = ::CFDataCreateWithBytesNoCopy(
    nullptr,
    first,
    last - first,
    kCFAllocatorNull
  );

  impl_.reset(::SecCertificateCreateWithData(nullptr, data.ref));
  if (impl_)
  {
    error.clear();
  }
  else
  {
    error = std::make_error_code(std::errc::illegal_byte_sequence);
  }
}


uint8_t *certificate_t::to_der (uint8_t *first, uint8_t *last,
  std::error_code &error) const noexcept
{
  if (scoped_ref<CFDataRef> data = ::SecCertificateCopyData(impl_.ref))
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
  if (scoped_ref<CFDataRef> data = ::SecCertificateCopyData(impl_.ref))
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
    return atoi(c_str(value.ref, buf));
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

  scoped_ref<CFDataRef> value = ::SecCertificateCopySerialNumber(
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


bool certificate_t::issued_by (const certificate_t &issuer,
  std::error_code &error) const noexcept
{
  if (!impl_ || !issuer.impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }
  error.clear();

  scoped_ref<CFDataRef> issuer_cert_subject_data =
    ::SecCertificateCopyNormalizedSubjectSequence(
      issuer.impl_.ref
    );

  scoped_ref<CFDataRef> this_cert_issuer_data =
    ::SecCertificateCopyNormalizedIssuerSequence(
      impl_.ref
    );

  auto size_1 = ::CFDataGetLength(issuer_cert_subject_data.ref);
  auto size_2 = ::CFDataGetLength(this_cert_issuer_data.ref);
  if (size_1 != size_2)
  {
    return false;
  }

  auto data_1 = ::CFDataGetBytePtr(issuer_cert_subject_data.ref);
  auto data_2 = ::CFDataGetBytePtr(this_cert_issuer_data.ref);
  return std::equal(data_1, data_1 + size_1, data_2);
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

  char buf[64];
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


std::vector<std::pair<certificate_t::alt_name, std::string>> to_alt_name (
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
          result.emplace_back(certificate_t::alt_name::dns,
            c_str(::CFDictionaryGetValue(entry, kSecPropertyKeyValue), buf)
          );
        }
        else if (::CFEqual(label, ip_address))
        {
          result.emplace_back(certificate_t::alt_name::ip,
            normalized_ip_string(::CFDictionaryGetValue(entry, kSecPropertyKeyValue))
          );
        }
        else if (::CFEqual(label, uri))
        {
          result.emplace_back(certificate_t::alt_name::uri,
            c_str(::CFURLGetString((CFURLRef)::CFDictionaryGetValue(entry, kSecPropertyKeyValue)), buf)
          );
        }
        else if (::CFEqual(label, email_address))
        {
          result.emplace_back(certificate_t::alt_name::email,
            c_str(::CFDictionaryGetValue(entry, kSecPropertyKeyValue), buf)
          );
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


} // namespace


std::vector<std::pair<certificate_t::alt_name, std::string>>
certificate_t::issuer_alt_name (std::error_code &error) const noexcept
{
  return to_alt_name(impl_.ref, kSecOIDIssuerAltName, error);
}


std::vector<std::pair<certificate_t::alt_name, std::string>>
certificate_t::subject_alt_name (std::error_code &error) const noexcept
{
  return to_alt_name(impl_.ref, kSecOIDSubjectAltName, error);
}


#elif __sal_os_linux //{{{1


certificate_t::certificate_t (const uint8_t *first, const uint8_t *last,
  std::error_code &error) noexcept
{
  if (last <= first)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return;
  }

  impl_ = d2i_X509(nullptr, &first, (last - first));
  if (impl_)
  {
    error.clear();
  }
  else
  {
    error = std::make_error_code(std::errc::illegal_byte_sequence);
  }
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
    error = std::make_error_code(std::errc::illegal_byte_sequence);
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


bool certificate_t::issued_by (const certificate_t &issuer,
  std::error_code &error) const noexcept
{
  if (!impl_ || !issuer.impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  error.clear();
  return X509_check_issued(issuer.impl_.ref, impl_.ref) == X509_V_OK;
}


namespace {


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

      result.emplace_back(oid,
        reinterpret_cast<const char *>(
          ASN1_STRING_data(X509_NAME_ENTRY_get_data(entry))
        )
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
        result.emplace_back(filter_oid,
          reinterpret_cast<const char *>(
            ASN1_STRING_data(X509_NAME_ENTRY_get_data(entry))
          )
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

std::vector<std::pair<certificate_t::alt_name, std::string>> to_alt_name (
  X509 *cert, int nid, std::error_code &error) noexcept
{
  std::vector<std::pair<certificate_t::alt_name, std::string>> result;

  if (!cert)
  {
    error = std::make_error_code(std::errc::bad_address);
    return result;
  }

  auto names = X509_get_ext_d2i(cert, nid, nullptr, nullptr);
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
            result.emplace_back(certificate_t::alt_name::email,
              std::string{s->data, s->data + s->length}
            );
          }
          break;
        }

        case GEN_DNS:
        {
          auto s = name->d.dNSName;
          if (s && s->type == V_ASN1_IA5STRING && s->data && s->length > 0)
          {
            result.emplace_back(certificate_t::alt_name::dns,
              std::string{s->data, s->data + s->length}
            );
          }
          break;
        }

        case GEN_URI:
        {
          auto s = name->d.uniformResourceIdentifier;
          if (s && s->type == V_ASN1_IA5STRING && s->data && s->length > 0)
          {
            result.emplace_back(certificate_t::alt_name::uri,
              std::string{s->data, s->data + s->length}
            );
          }
          break;
        }

        case GEN_IPADD:
        {
          auto s = name->d.iPAddress;
          if (s && s->type == V_ASN1_OCTET_STRING && s->data
            && (s->length == 4 || s->length == 16))
          {
            result.emplace_back(certificate_t::alt_name::ip,
              normalized_ip_string(s->data, s->length)
            );
          }
          break;
        }

        default:
          continue;
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
certificate_t::issuer_alt_name (std::error_code &error) const noexcept
{
  return to_alt_name(impl_.ref, NID_issuer_alt_name, error);
}


std::vector<std::pair<certificate_t::alt_name, std::string>>
certificate_t::subject_alt_name (std::error_code &error) const noexcept
{
  return to_alt_name(impl_.ref, NID_subject_alt_name, error);
}


#elif __sal_os_windows // {{{1


certificate_t::certificate_t (const uint8_t *first, const uint8_t *last,
  std::error_code &error) noexcept
{
  if (last <= first)
  {
    error = std::make_error_code(std::errc::invalid_argument);
    return;
  }

  impl_.reset(
    ::CertCreateCertificateContext(
      X509_ASN_ENCODING,
      first,
      static_cast<DWORD>(last - first)
    )
  );
  if (impl_)
  {
    error.clear();
  }
  else
  {
    error = std::make_error_code(std::errc::illegal_byte_sequence);
  }
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

  error = std::make_error_code(std::errc::illegal_byte_sequence);
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


bool certificate_t::issued_by (const certificate_t &issuer,
  std::error_code &error) const noexcept
{
  if (!impl_ || !issuer.impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  error.clear();
  return ::CertCompareCertificateName(
    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
    &issuer.impl_.ref->pCertInfo->Subject,
    &impl_.ref->pCertInfo->Issuer
  );
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


std::vector<std::pair<certificate_t::alt_name, std::string>> to_alt_name (
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

        default:
          continue;
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
certificate_t::issuer_alt_name (std::error_code &error) const noexcept
{
  return to_alt_name(impl_.ref, szOID_ISSUER_ALT_NAME2, error);
}


std::vector<std::pair<certificate_t::alt_name, std::string>>
certificate_t::subject_alt_name (std::error_code &error) const noexcept
{
  return to_alt_name(impl_.ref, szOID_SUBJECT_ALT_NAME2, error);
}


#endif // }}}1


} // namespace crypto


__sal_end
