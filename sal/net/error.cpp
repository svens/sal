#include <sal/net/error.hpp>


__sal_begin


namespace net { namespace ip {


namespace {

class resolver_category_t
  : public std::error_category
{
public:

  const char *name () const noexcept final override
  {
    return "sal::net::ip::resolver";
  }

  std::string message (int value) const final override
  {
    switch (static_cast<resolver_errc_t>(value))
    {
      case resolver_errc_t::host_not_found:
        return "host not found";
      case resolver_errc_t::host_not_found_try_again:
        return "host not found, try again";
      case resolver_errc_t::service_not_found:
        return "service not found";
    }
    return "unknown sal::net::ip::resolver error";
  }
};

} // namespace


const std::error_category &resolver_category () noexcept
{
  static const resolver_category_t cat_{};
  return cat_;
}


}} // namespace net::ip


__sal_end
