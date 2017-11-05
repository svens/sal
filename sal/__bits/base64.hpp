#pragma once

#include <sal/config.hpp>
#include <system_error>


__sal_begin


namespace __bits {


struct base64
{
  static size_t max_encoded_size (const uint8_t *first,
    const uint8_t *last) noexcept
  {
    return (((last - first) * 4 / 3) + 3) & ~3;

  }

  static size_t max_decoded_size (const uint8_t *first,
    const uint8_t *last,
    std::error_code &error) noexcept
  {
    if ((last - first) % 4 == 0)
    {
      error.clear();
      return (last - first) / 4 * 3;
    }
    error = std::make_error_code(std::errc::message_size);
    return 0;
  }


  static uint8_t *encode (
    const uint8_t *first,
    const uint8_t *last,
    uint8_t *out
  ) noexcept;


  static uint8_t *decode (
    const uint8_t *first,
    const uint8_t *last,
    uint8_t *out,
    std::error_code &error
  ) noexcept;
};


} // namespace __bits


__sal_end
