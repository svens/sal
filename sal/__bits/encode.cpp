#include <sal/__bits/encode.hpp>


__sal_begin


namespace __bits {


char *hex_string::encode (const char *first, const char *last, char *out)
  noexcept
{
  static constexpr char digits[] = "0123456789abcdef";

  while (first != last)
  {
    *out++ = digits[*first >> 4];
    *out++ = digits[*first & 0xf];
    ++first;
  }

  return out;
}


char *base64::encode (const char *first, const char *last, char *out) noexcept
{
  static constexpr char b64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/";

  auto mod = (last - first) % 3;
  last -= mod;

  while (first != last)
  {
    *out++ = b64[first[0] >> 2];
    *out++ = b64[((first[0] << 4) | (first[1] >> 4)) & 0x3f];
    *out++ = b64[((first[1] << 2) | (first[2] >> 6)) & 0x3f];
    *out++ = b64[first[2] & 0x3f];
    first += 3;
  }

  if (mod == 1)
  {
    *out++ = b64[first[0] >> 2];
    *out++ = b64[((first[0] << 4) | (first[1] >> 4)) & 0x3f];
    *out++ = '=';
    *out++ = '=';
  }
  else if (mod == 2)
  {
    *out++ = b64[first[0] >> 2];
    *out++ = b64[((first[0] << 4) | (first[1] >> 4)) & 0x3f];
    *out++ = b64[(first[1] << 2) & 0x3f];
    *out++ = '=';
  }

  return out;
}


} // namespace __bits


__sal_end
