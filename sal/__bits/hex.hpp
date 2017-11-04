#pragma once

#include <sal/config.hpp>
#include <system_error>


__sal_begin


namespace __bits {


struct hex_string
{
  template <typename It>
  static constexpr void expect_sizeof_1 (It first) noexcept
  {
    static_assert(sizeof(*first) == sizeof(uint8_t),
      "expected It element sizeof = 1"
    );
  }


  template <typename It>
  static constexpr size_t max_encoded_size (It first, It last) noexcept
  {
    expect_sizeof_1(first);
    return 2 * (last - first);
  }


  template <typename It>
  static constexpr size_t max_decoded_size (It first, It last,
    std::error_code &error) noexcept
  {
    expect_sizeof_1(first);
    if ((last - first) % 2 == 0)
    {
      error.clear();
      return (last - first) / 2;
    }
    error = std::make_error_code(std::errc::message_size);
    return 0;
  }


  template <typename InputIt, typename ForwardIt>
  static constexpr ForwardIt encode (InputIt first, InputIt last,
    ForwardIt out
  ) noexcept;


  template <typename InputIt, typename ForwardIt>
  static constexpr ForwardIt decode (InputIt first, InputIt last,
    ForwardIt out,
    std::error_code &error
  ) noexcept;


  static constexpr const char digits[] = "0123456789abcdef";

  static constexpr const char lookup[] =
    // _0  _1  _2  _3  _4  _5  _6  _7  _8  _9  _a  _b  _c  _d  _e  _f
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 0_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 1_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 2_
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\xff\xff\xff\xff\xff\xff" // 3_
    "\xff\x0a\x0b\x0c\x0d\x0e\x0f\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 4_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 5_
    "\xff\x0a\x0b\x0c\x0d\x0e\x0f\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 6_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 7_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 8_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // 9_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // a_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // b_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // c_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // d_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // e_
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff" // f_
  ;
};


template <typename InputIt, typename ForwardIt>
constexpr ForwardIt hex_string::encode (InputIt first, InputIt last,
  ForwardIt out) noexcept
{
  expect_sizeof_1(first);
  expect_sizeof_1(out);

  while (first != last)
  {
    *out++ = digits[*first >> 4];
    *out++ = digits[*first & 0xf];
    ++first;
  }

  return out;
}


template <typename InputIt, typename ForwardIt>
constexpr ForwardIt hex_string::decode (InputIt first, InputIt last,
  ForwardIt out,
  std::error_code &error) noexcept
{
  expect_sizeof_1(first);
  expect_sizeof_1(out);

  if ((last - first) % 2 != 0)
  {
    error = std::make_error_code(std::errc::message_size);
    return out;
  }

  uint8_t hi{}, lo{};
  for (/**/;  first != last;  first += 2)
  {
    hi = lookup[size_t(first[0])];
    lo = lookup[size_t(first[1])];
    if (hi != 0xff && lo != 0xff)
    {
      *out++ = (hi << 4) | lo;
    }
    else
    {
      break;
    }
  }

  if (hi != 0xff && lo != 0xff)
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
