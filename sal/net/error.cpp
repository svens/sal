#include <sal/net/error.hpp>


__sal_begin


namespace net { namespace ip {


namespace {

class resolver_category_impl_t
  : public std::error_category
{
public:

  const char *name () const noexcept final override
  {
    return "resolver";
  }

  std::string message (int value) const final override
  {
    auto m = gai_strerror(value);
    return std::string(m && *m ? m : "Unknown error");
  }
};

} // namespace


const std::error_category &resolver_category () noexcept
{
  static const resolver_category_impl_t cat_{};
  return cat_;
}


}} // namespace net::ip


__sal_end
