#pragma once

/**
 * \file sal/net/fwd.hpp
 * Forward declarations
 */


#include <sal/config.hpp>
#include <system_error>


__sal_begin


namespace net {


/**
 * On Windows platform, initialise winsock library. There is no need to call
 * it explicitly as it is also done internally by static initialisation. Only
 * exception is when application layer own static initialisation order depends
 * on winsock library to be already loaded. It can be called multiple times.
 * \c std::call_once is used to make sure only single call proceeds.
 */
const std::error_code &init () noexcept;


} // namespace net


__sal_end
