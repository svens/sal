#include <sal/uri_error.hpp>
#include <string_view>


__sal_begin


namespace {


constexpr std::string_view as_view (uri_errc value) noexcept
{
  switch (static_cast<uri_errc>(value))
  {
    case uri_errc::invalid_scheme:
      return "invalid scheme";
    case uri_errc::invalid_authority:
      return "invalid authority";
    case uri_errc::invalid_path:
      return "invalid path";
    case uri_errc::invalid_query:
      return "invalid query";
    case uri_errc::invalid_fragment:
      return "invalid fragment";
  }
  return "Unknown error";
}


class uri_category_impl_t
  : public std::error_category
{
  const char *name () const noexcept final override
  {
    return "uri";
  }

  std::string message (int value) const final override
  {
    return std::string{as_view(static_cast<uri_errc>(value))};
  }
};


} // namespace


const std::error_category &uri_category () noexcept
{
  static const uri_category_impl_t cat_{};
  return cat_;
}


__sal_end
