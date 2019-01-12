#pragma once

/**
 * \file sal/uri/scheme.hpp
 * URI scheme traits
 */

#include <sal/config.hpp>
#include <sal/uri/view.hpp>
#include <sal/net/fwd.hpp>
#include <string_view>


__sal_begin


namespace uri {


struct scheme_t
{
  const net::ip::port_t default_port;
  const std::string_view default_path;
  const bool case_insensitive_path;
};


const scheme_t &generic_scheme () noexcept;
const scheme_t &mailto_scheme () noexcept;
const scheme_t &ftp_scheme () noexcept;
const scheme_t &http_scheme () noexcept;
const scheme_t &https_scheme () noexcept;


} // namespace uri


__sal_end
