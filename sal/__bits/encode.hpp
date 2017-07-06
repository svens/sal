#pragma once

#include <sal/config.hpp>


__sal_begin


namespace __bits {


struct hex_string
{
  static size_t max_encoded_size (const char *first, const char *last)
    noexcept
  {
    return 2 * (last - first);
  }

  static char *encode (const char *first, const char *last, char *out)
    noexcept;
};


struct base64
{
  static size_t max_encoded_size (const char *first, const char *last)
    noexcept
  {
    return ((last - first) + 2) / 3 * 4;
  }

  static char *encode (const char *first, const char *last, char *out)
    noexcept;
};


} // namespace __bits


__sal_end
