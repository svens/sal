#include <sal/__bits/base64.hpp>


__sal_begin


namespace __bits {


namespace {

static constexpr unsigned char digits[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789"
  "+/";

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

} // namespace


uint8_t *base64::encode (
  const uint8_t *first,
  const uint8_t *last,
  uint8_t *out) noexcept
{
  auto mod = (last - first) % 3;
  last -= mod;

  for (/**/;  first != last;  first += 3)
  {
    *out++ = digits[first[0] >> 2];
    *out++ = digits[((first[0] << 4) | (first[1] >> 4)) & 0b00111111];
    *out++ = digits[((first[1] << 2) | (first[2] >> 6)) & 0b00111111];
    *out++ = digits[first[2] & 0b00111111];
  }

  if (mod == 1)
  {
    *out++ = digits[first[0] >> 2];
    *out++ = digits[((first[0] << 4)) & 0b00111111];
    *out++ = '=';
    *out++ = '=';
  }
  else if (mod == 2)
  {
    *out++ = digits[first[0] >> 2];
    *out++ = digits[((first[0] << 4) | (first[1] >> 4)) & 0b00111111];
    *out++ = digits[(first[1] << 2) & 0b00111111];
    *out++ = '=';
  }

  return out;
}


uint8_t *base64::decode (
  const uint8_t *first,
  const uint8_t *last,
  uint8_t *out,
  std::error_code &error) noexcept
{
  if ((last - first) % 4 != 0)
  {
    error = std::make_error_code(std::errc::message_size);
    return out;
  }

  auto pad = (last[-1] == '=') + (last[-2] == '=');
  last = first + ((last - first - pad) / 4) * 4;

  uint8_t b0{}, b1{}, b2{}, b3{};
  for (/**/;  first != last;  first += 4)
  {
    b0 = lookup[size_t(first[0])];
    b1 = lookup[size_t(first[1])];
    b2 = lookup[size_t(first[2])];
    b3 = lookup[size_t(first[3])];
    if (b0 != 0xff && b1 != 0xff && b2 != 0xff && b3 != 0xff)
    {
      auto v = (b0 << 18) | (b1 << 12) | (b2 << 6) | b3;
      *out++ = (v >> 16) & 0xff;
      *out++ = (v >> 8) & 0xff;
      *out++ = v & 0xff;
    }
    else
    {
      break;
    }
  }

  if (pad == 1)
  {
    b0 = lookup[size_t(first[0])];
    b1 = lookup[size_t(first[1])];
    b2 = lookup[size_t(first[2])];
    if (b0 != 0xff && b1 != 0xff && b2 != 0xff)
    {
      auto v = (b0 << 18) | (b1 << 12) | (b2 << 6);
      *out++ = (v >> 16) & 0xff;
      *out++ = (v >> 8) & 0xff;
    }
  }
  else if (pad == 2)
  {
    b0 = lookup[size_t(first[0])];
    b1 = lookup[size_t(first[1])];
    if (b0 != 0xff && b1 != 0xff)
    {
      *out++ = ((b0 << 18) | (b1 << 12)) >> 16;
    }
  }

  if (b0 != 0xff && b1 != 0xff && b2 != 0xff && b3 != 0xff)
  {
    error.clear();
  }
  else
  {
    error = std::make_error_code(std::errc::illegal_byte_sequence);
  }

  return out;
}


} // namespace __bits


__sal_end
