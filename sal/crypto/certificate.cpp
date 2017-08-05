#include <sal/crypto/certificate.hpp>
#include <sal/encode.hpp>
#include <sstream>


#if __sal_os_darwin //{{{1
  #if !defined(__apple_build_version__)
    #define availability(...) /**/
  #endif
  #include <CoreFoundation/CFNumber.h>
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


scoped_ref<CFArrayRef> copy_values (SecCertificateRef cert, CFTypeRef oid)
  noexcept
{
  scoped_ref<CFArrayRef> keys = ::CFArrayCreate(nullptr, &oid, 1, nullptr);
  scoped_ref<CFDictionaryRef> dir = ::SecCertificateCopyValues(cert, keys.ref, nullptr);

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

  // LCOV_EXCL_START
  return {};
  // LCOV_EXCL_STOP
}


auto to_distinguished_name (SecCertificateRef cert, CFTypeRef oid,
  std::error_code &error) noexcept
{
  certificate_t::distinguished_name_t result;

  if (!cert)
  {
    error = std::make_error_code(std::errc::bad_address);
    return result;
  }

  if (auto values = copy_values(cert, oid))
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

  if (!cert)
  {
    error = std::make_error_code(std::errc::bad_address);
    return result;
  }

  if (auto values = copy_values(cert, oid))
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
        if (::CFStringCompare(label, filter.ref, 0) == kCFCompareEqualTo)
        {
          char buf[1024];
          result.emplace_back(
            c_str(label, buf),
            c_str(::CFDictionaryGetValue(key, kSecPropertyKeyValue), buf)
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


sal::time_t to_time (SecCertificateRef cert, CFTypeRef oid,
  std::error_code &error) noexcept
{
  if (!cert)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  if (auto value = copy_values(cert, oid))
  {
    CFAbsoluteTime time;
    ::CFNumberGetValue((CFNumberRef)value.ref, kCFNumberDoubleType, &time);

    error.clear();
    return sal::clock_t::from_time_t(time + kCFAbsoluteTimeIntervalSince1970);
  }

  // LCOV_EXCL_START
  error = std::make_error_code(std::errc::illegal_byte_sequence);
  return {};
  // LCOV_EXCL_STOP
}


} // namespae


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

  try
  {
    std::vector<uint8_t> result(first, last);
    error.clear();
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


bool certificate_t::issued_by (const certificate_t &issuer,
  std::error_code &error) const noexcept
{
  if (!impl_ || !issuer.impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

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

  error.clear();
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


#endif // }}}1


} // namespace crypto


__sal_end
