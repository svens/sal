#include <sal/__bits/encode.hpp>


__sal_begin


namespace __bits {


// hex_string {{{1


uint8_t *hex_string::encode (const uint8_t *first, const uint8_t *last,
  uint8_t *out) noexcept
{
  static constexpr uint8_t digits[] = "0123456789abcdef";

  while (first != last)
  {
    *out++ = digits[*first >> 4];
    *out++ = digits[*first & 0xf];
    ++first;
  }

  return out;
}


namespace {

struct hex_lookup_t
{
  uint8_t table[256];

  constexpr hex_lookup_t ()
    : table{}
  {
    for (auto &ch: table)
    {
      ch = 0xff;
    }
    for (int i = 0;  i != 16;  ++i)
    {
      table[static_cast<size_t>("0123456789abcdef"[i])] = static_cast<uint8_t>(i);
    }
    for (int i = 0;  i != 6;  ++i)
    {
      table[static_cast<size_t>("ABCDEF"[i])] = static_cast<uint8_t>(i + 10);
    }
  }

  constexpr uint8_t operator[] (uint8_t i) const
  {
    return table[static_cast<size_t>(i)];
  }
};
static constexpr hex_lookup_t hex_{};

} // namespace


uint8_t *hex_string::decode (const uint8_t *first, const uint8_t *last,
  uint8_t *out, std::error_code &error) noexcept
{
  if ((last - first) % 2 != 0)
  {
    error = std::make_error_code(std::errc::message_size);
    return out;
  }

  for (uint8_t hi, lo;  first != last;  first += 2)
  {
    hi = hex_[first[0]];
    lo = hex_[first[1]];
    if (hi != 0xff && lo != 0xff)
    {
      *out++ = (hi << 4) | lo;
    }
    else
    {
      error = std::make_error_code(std::errc::illegal_byte_sequence);
      return out;
    }
  }

  error.clear();
  return out;
}


// base64 {{{1


namespace {


static constexpr uint8_t base64_encode[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789"
  "+/";


struct base64_lookup_t
{
  uint8_t table[256];

  constexpr base64_lookup_t ()
    : table{}
  {
    for (auto &ch: table)
    {
      ch = 0xff;
    }
    for (int i = 0;  i != 64;  ++i)
    {
      table[static_cast<size_t>(base64_encode[i])] = static_cast<uint8_t>(i);
    }
  }

  constexpr uint8_t operator[] (uint8_t i) const
  {
    return table[static_cast<size_t>(i)];
  }
};
static constexpr base64_lookup_t base64_decode{};


} // namespace


uint8_t *base64::encode (const uint8_t *first, const uint8_t *last,
  uint8_t *out) noexcept
{
  auto mod = (last - first) % 3;
  last -= mod;

  for (/**/;  first != last;  first += 3)
  {
    *out++ = base64_encode[first[0] >> 2];
    *out++ = base64_encode[((first[0] << 4) | (first[1] >> 4)) & 0b00111111];
    *out++ = base64_encode[((first[1] << 2) | (first[2] >> 6)) & 0b00111111];
    *out++ = base64_encode[first[2] & 0b00111111];
  }

  if (mod == 1)
  {
    *out++ = base64_encode[first[0] >> 2];
    *out++ = base64_encode[((first[0] << 4) | (first[1] >> 4)) & 0b00111111];
    *out++ = '=';
    *out++ = '=';
  }
  else if (mod == 2)
  {
    *out++ = base64_encode[first[0] >> 2];
    *out++ = base64_encode[((first[0] << 4) | (first[1] >> 4)) & 0b00111111];
    *out++ = base64_encode[(first[1] << 2) & 0b00111111];
    *out++ = '=';
  }

  return out;
}


uint8_t *base64::decode (const uint8_t *first, const uint8_t *last, uint8_t *out,
  std::error_code &error) noexcept
{
  if ((last - first) % 4 != 0)
  {
    error = std::make_error_code(std::errc::message_size);
    return out;
  }

  for (int32_t val = 0, valb = -8;  first != last;  ++first)
  {
    if (base64_decode[*first] == 0xff)
    {
      if (((last - first) == 1 && first[0] == '=')
        || (last - first == 2 && first[0] == '=' && first[1] == '='))
      {
        break;
      }
      error = std::make_error_code(std::errc::illegal_byte_sequence);
      return out;
    }

    val = (val << 6) | base64_decode[*first];
    valb += 6;

    if (valb >= 0)
    {
      *out++ = static_cast<uint8_t>((val >> valb) & 0xff);
      valb -= 8;
    }
  }

  error.clear();
  return out;
}


// }}}1


} // namespace __bits


__sal_end
