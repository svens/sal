#include <sal/crypto/__bits/x509.hpp>
#include <algorithm>
#include <cctype>


__sal_begin


namespace crypto::__bits {


namespace {


uint8_t *base64_decode (const uint8_t *first, const uint8_t *last, uint8_t *d)
  noexcept
{
  static constexpr unsigned char lookup[] =
    // _0  _1  _2  _3  _4  _5  _6  _7  _8  _9  _a  _b  _c  _d  _e  _f
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 0_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 1_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x3e\xff\xff\xff\x3f" // 2_
    "\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\xff\xff\xff\xff\xff\xff" // 3_
    "\xff\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e" // 4_
    "\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\xff\xff\xff\xff\xff" // 5_
    "\xff\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27\x28" // 6_
    "\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\xff\xff\xff\xff\xff" // 7_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 8_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 9_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // a_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // b_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // c_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // d_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // e_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // f_
  ;

  for (int32_t val = 0, valb = -8;  first != last;  ++first)
  {
    if (lookup[*first] != 0xff)
    {
      val = (val << 6) | lookup[*first];
      if ((valb += 6) >= 0)
      {
        *d++ = static_cast<uint8_t>((val >> valb) & 0xff);
        valb -= 8;
      }
    }
    else if (std::isspace(*first))
    {
      continue;
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
    // decode (can't use sal::encode<base64> because of whitespaces)
    return base64_decode(first, last, der_first);
  }

  // no room or invalid data
  return {};
}


} // namespace crypto::__bits


__sal_end
