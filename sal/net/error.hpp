#pragma once

/**
 * \file sal/net/error.hpp
 */


#include <sal/config.hpp>
#include <sal/error.hpp>
#include <typeinfo>


__sal_begin


namespace net {
namespace ip {


/**
 * Exception type thrown when invalid address casting between IPv4 and IPv6 is
 * attempted.
 */
class bad_address_cast_t
  : public std::bad_cast
{
};


namespace __bits {

inline void bad_address_cast [[noreturn]] ()
{
  throw bad_address_cast_t{};
}

} // namespace __bits


} // namespace ip
} // namespace net


__sal_end
