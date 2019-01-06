#include <sal/uri/scheme.hpp>


__sal_begin


namespace uri {


const scheme_t &generic_scheme () noexcept
{
  static const scheme_t impl
  {
    // .default_port =
    0,
    // .default_path =
    {},
  };
  return impl;
}


const scheme_t &ftp_scheme () noexcept
{
  static const scheme_t impl
  {
    // .default_port =
    21,
    // .default_path =
    "/",
  };
  return impl;
}


const scheme_t &http_scheme () noexcept
{
  static const scheme_t impl
  {
    // .default_port =
    80,
    // .default_path =
    "/",
  };
  return impl;
}


const scheme_t &https_scheme () noexcept
{
  static const scheme_t impl
  {
    // .default_port =
    443,
    // .default_path =
    "/",
  };
  return impl;
}


} // namespace uri


__sal_end
