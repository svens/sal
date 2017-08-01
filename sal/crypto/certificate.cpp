#include <sal/crypto/certificate.hpp>
#include <sal/encode.hpp>
#include <sstream>


#if __sal_os_darwin //{{{1
  #if !defined(__apple_build_version__)
    #define availability(...) /**/
  #endif
  #include <Security/SecCertificateOIDs.h>
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

  return {};
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


std::string certificate_t::display_name (std::error_code &error)
  const noexcept
{
  if (impl_)
  {
    try
    {
      scoped_ref<CFStringRef> s = ::SecCertificateCopySubjectSummary(impl_.ref);
      char buf[1024];
      error.clear();
      return c_str(s.ref, buf);
    }

    // LCOV_EXCL_START
    catch (const std::bad_alloc &)
    {
      error = std::make_error_code(std::errc::not_enough_memory);
      return {};
    }
    // LCOV_EXCL_STOP
  }

  error = std::make_error_code(std::errc::bad_address);
  return {};
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


std::string certificate_t::display_name (std::error_code &error)
  const noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  try
  {
    // TODO
    return {};
  }

  // LCOV_EXCL_START
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return {};
  }
  // LCOV_EXCL_STOP
}


namespace {


auto to_distinguished_name (X509_NAME *name, std::error_code &error)
  noexcept
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

  // LCOV_EXCL_START
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    return {};
  }
  // LCOV_EXCL_STOP
}


#if 0
namespace {


std::string get_name (PCCERT_CONTEXT cert,
  DWORD type,
  DWORD flags,
  void *params,
  std::error_code &error) noexcept
{
  if (!cert)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  try
  {
    auto size = ::CertGetNameString(cert, type, flags, params, nullptr, 0);

    std::string result(size, {});
    ::CertGetNameString(cert, type, flags, params, &result[0], size);
    result.pop_back();

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


} // namespace


std::string certificate_t::display_name (std::error_code &error)
  const noexcept
{
  return get_name(impl_.ref,
    CERT_NAME_FRIENDLY_DISPLAY_TYPE,
    0,
    nullptr,
    error
  );
}


std::string certificate_t::issuer_name_impl (std::error_code &error)
  const noexcept
{
  DWORD string_type = CERT_X500_NAME_STR;
  return get_name(impl_.ref,
    CERT_NAME_RDN_TYPE,
    CERT_NAME_ISSUER_FLAG,
    &string_type,
    error
  );
}


std::string certificate_t::subject_name_impl (std::error_code &error)
  const noexcept
{
  DWORD string_type = CERT_X500_NAME_STR;
  return get_name(impl_.ref,
    CERT_NAME_RDN_TYPE,
    0,
    &string_type,
    error
  );
}
#endif


std::string certificate_t::display_name (std::error_code &error)
  const noexcept
{
  if (!impl_)
  {
    error = std::make_error_code(std::errc::bad_address);
    return {};
  }

  try
  {
    auto size = ::CertGetNameString(impl_.ref,
      CERT_NAME_FRIENDLY_DISPLAY_TYPE,
      0,
      nullptr,
      nullptr,
      0
    );

    std::string result(size, {});
    ::CertGetNameString(impl_.ref,
      CERT_NAME_FRIENDLY_DISPLAY_TYPE,
      0,
      nullptr,
      &result[0],
      size
    );
    result.pop_back();

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


certificate_t::distinguished_name_t certificate_t::issuer (
  std::error_code &error) const noexcept
{
  // TODO
  error.clear();
  return {};
}


certificate_t::distinguished_name_t certificate_t::issuer (const oid_t &oid,
  std::error_code &error) const noexcept
{
  // TODO
  (void)oid;
  error.clear();
  return {};
}


certificate_t::distinguished_name_t certificate_t::subject (
  std::error_code &error) const noexcept
{
  // TODO
  error.clear();
  return {};
}


certificate_t::distinguished_name_t certificate_t::subject (const oid_t &oid,
  std::error_code &error) const noexcept
{
  // TODO
  (void)oid;
  error.clear();
  return {};
}


#endif // }}}1


} // namespace crypto


__sal_end
