#include <sal/net/error.hpp>


__sal_begin


namespace net {


namespace {

class socket_category_impl_t
  : public std::error_category
{
public:

  const char *name () const noexcept final override
  {
    return "socket";
  }

  std::string message (int value) const final override
  {
    (void)value;
    return "hello";
  }
};

} // namespace


const std::error_category &socket_category () noexcept
{
  static const socket_category_impl_t cat_{};
  return cat_;
}



namespace ip {
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


} // namespace ip
} // namespace net


__sal_end
