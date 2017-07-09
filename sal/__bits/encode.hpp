#pragma once

#include <sal/config.hpp>
#include <system_error>


__sal_begin


namespace __bits {


struct hex_string
{
  static size_t max_encoded_size (const char *first, const char *last)
    noexcept
  {
    return 2 * (last - first);
  }

  static size_t max_decoded_size (const char *first, const char *last,
    std::error_code &error) noexcept
  {
    if ((last - first) % 2 == 0)
    {
      error.clear();
      return (last - first) / 2;
    }

    error = std::make_error_code(std::errc::message_size);
    return 0;
  }

  static char *encode (const char *first, const char *last, char *out)
    noexcept;

  static char *decode (const char *first, const char *last, char *out,
    std::error_code &error
  ) noexcept;
};


struct base64
{
  static size_t max_encoded_size (const char *first, const char *last)
    noexcept
  {
    return (((last - first) * 4 / 3) + 3) & ~3;
  }

  static size_t max_decoded_size (const char *first, const char *last,
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

  static char *encode (const char *first, const char *last, char *out)
    noexcept;

  static char *decode (const char *first, const char *last, char *out,
    std::error_code &error
  ) noexcept;
};


} // namespace __bits


__sal_end
