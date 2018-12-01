#pragma once

/**
 * \file sal/net/fwd.hpp
 * Forward declarations
 */


#include <sal/config.hpp>
#include <system_error>


__sal_begin


/// Networking primitives (sockets, resolver, etc)
namespace net {


// Socket
class socket_base_t;
template <typename Protocol> class basic_socket_t;
template <typename Protocol> class basic_datagram_socket_t;
template <typename Protocol> class basic_stream_socket_t;
template <typename Protocol> class basic_socket_acceptor_t;


// Error
enum class socket_errc;


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
enum class resolver_errc;

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


namespace async
{

class completion_queue_t;
class io_t;
class service_t;

} // namespace async


} // namespace net


__sal_end
