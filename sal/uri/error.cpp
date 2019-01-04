#include <sal/uri/error.hpp>
#include <string_view>


__sal_begin


namespace uri {


namespace {


constexpr std::string_view to_view (errc value) noexcept
{
  switch (static_cast<errc>(value))
  {
    case errc::invalid_scheme:
      return "invalid scheme";
    case errc::invalid_authority:
      return "invalid authority";
    case errc::invalid_port:
      return "invalid port";
    case errc::invalid_path:
      return "invalid path";
    case errc::invalid_query:
      return "invalid query";
    case errc::invalid_fragment:
      return "invalid fragment";
    case errc::invalid_hex_input:
      return "invalid hex input";
    case errc::not_enough_input:
      return "not enough input";
  }
  return "Unknown error";
}


class category_impl_t
  : public std::error_category
{
  const char *name () const noexcept final override
  {
    return "uri";
  }

  std::string message (int value) const final override
  {
    return std::string{to_view(static_cast<errc>(value))};
  }
};


} // namespace


const std::error_category &category () noexcept
{
  static const category_impl_t cat_{};
  return cat_;
}


} // namespace uri


__sal_end
