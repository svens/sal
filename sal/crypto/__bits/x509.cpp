#include <sal/crypto/__bits/x509.hpp>
#include <algorithm>
#include <cctype>


__sal_begin


namespace crypto { namespace __bits {


namespace {

struct base64_t
{
  static constexpr uint8_t encode[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/";

  uint8_t decode[256];

  constexpr base64_t ()
    : decode{}
  {
    for (auto &i: decode)
    {
      i = 0xff;
    }
    for (int i = 0;  i != 64;  ++i)
    {
      decode[static_cast<size_t>(encode[i])] = static_cast<uint8_t>(i);
    }
  }
};

uint8_t *base64_decode (const uint8_t *first, const uint8_t *last, uint8_t *d)
  noexcept
{
  static constexpr base64_t base64;

  for (int32_t val = 0, valb = -8;  first != last;  ++first)
  {
    if (std::isspace(*first))
    {
      continue;
    }
    else if (base64.decode[*first] != 0xff)
    {
      val = (val << 6) | base64.decode[*first];
      if ((valb += 6) >= 0)
      {
        *d++ = static_cast<uint8_t>((val >> valb) & 0xff);
        valb -= 8;
      }
    }
    else if ((last - first == 1 && first[0] == '=')
      || (last - first == 2 && first[0] == '=' && first[1] == '='))
    {
      break;
    }
    else
    {
      return {};
    }
  }

  return d;
}

} // namepsace


uint8_t *pem_to_der (const uint8_t *pem_first, const uint8_t *pem_last,
  uint8_t *der_first, uint8_t *der_last) noexcept
{
  static constexpr char
    BEGIN_[] = "-----BEGIN",
    END_[] = "-----END";

  static constexpr const char
    *_BEGIN = BEGIN_ + sizeof(BEGIN_) - 1,
    *_END = END_ + sizeof(END_) - 1;

  // skip BEGIN
  auto first = std::search(pem_first, pem_last, BEGIN_, _BEGIN);
  if (first != pem_first || first == pem_last)
  {
    return {};
  }

  // skip until end of line after BEGIN
  first += sizeof(BEGIN_) - 1;
  while (first != pem_last && *first != '\n')
  {
    ++first;
  }
  if (first == pem_last)
  {
    return {};
  }

  // find first END
  auto last = std::search(first, pem_last, END_, _END);
  if (last == pem_last)
  {
    return {};
  }

  // move backward until end of line
  while (last != first && *last != '\n')
  {
    --last;
  }
  if (first == last)
  {
    return {};
  }

  auto max_decoded_size = (last - first) / 4 * 3;
  if (der_last - der_first >= max_decoded_size)
  {
    // decode (can't use sal::encode<base64> because of whitepsaces)
    return base64_decode(first, last, der_first);
  }

  // no room or invalid data
  return {};
}


}} // namespace crypto::__bits


__sal_end
