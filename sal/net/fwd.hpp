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


// Error
enum class socket_errc_t;
class throw_on_error;


// Socket
class socket_base_t;
template <typename Protocol> class basic_socket_t;
template <typename Protocol> class basic_datagram_socket_t;
template <typename Protocol> class basic_stream_socket_t;
template <typename Protocol> class basic_socket_acceptor_t;


namespace ip {

/// Port number
using port_t =  uint_least16_t;

/// IPv6 endpoint scope id
using scope_id_t = uint_least32_t;

// Address
class address_t;
class address_v4_t;
class address_v6_t;

// Error
class bad_address_cast_t;
enum class resolver_errc_t;

// Protocol
class tcp_t;
class udp_t;

// Endpoint
template <typename Protocol> class basic_endpoint_t;

// Resolver
class resolver_base_t;
template <typename Protocol> class basic_resolver_entry_t;
template <typename Protocol> class basic_resolver_results_iterator_t;
template <typename Protocol> class basic_resolver_results_t;
template <typename Protocol> class basic_resolver_t;


} // namespace ip


} // namespace net


__sal_end
