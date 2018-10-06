#pragma once

/**
 * \file sal/net/ip/basic_resolver.hpp
 * Internet endpoint resolver
 */


#include <sal/config.hpp>
#include <sal/net/ip/__bits/inet.hpp>
#include <sal/net/error.hpp>
#include <sal/net/ip/basic_resolver_entry.hpp>
#include <sal/net/ip/basic_resolver_results.hpp>
#include <sal/net/ip/resolver_base.hpp>


__sal_begin


namespace net::ip {


/**
 * Translator of host and/or service name to set of endpoints.
 */
template <typename Protocol>
class basic_resolver_t
  : public resolver_base_t
{
public:

  /**
   * Protocol type
   */
  using protocol_t = Protocol;

  /**
   * Endpoint type
   */
  using endpoint_t = typename Protocol::endpoint_t;

  /**
   * Translation result set.
   */
  using results_t = basic_resolver_results_t<Protocol>;


  //
  // resolve (const char *)
  //

  /**
   * Translate \a host_name and/or \a service_name using specified \a flags.
   * On failure, set \a error and return empty result set.
   */
  results_t resolve (const char *host_name,
    const char *service_name,
    flags_t flags,
    std::error_code &error
  ) noexcept;


  /**
   * Translate \a host_name and/or \a service_name. On failure, set \a error
   * and return empty result set.
   */
  results_t resolve (const char *host_name,
    const char *service_name,
    std::error_code &error) noexcept
  {
    return resolve(host_name, service_name, flags_t(), error);
  }


  /**
   * Translate \a host_name and/or \a service_name using specified \a flags.
   * On failure, throw std::system_error.
   */
  results_t resolve (const char *host_name,
    const char *service_name,
    flags_t flags)
  {
    return resolve(host_name, service_name, flags, throw_on_error("resolve"));
  }


  /**
   * Translate \a host_name and/or \a service_name. On failure, throw
   * std::system_error.
   */
  results_t resolve (const char *host_name,
    const char *service_name)
  {
    return resolve(host_name, service_name, flags_t());
  }


  //
  // resolve (protocol_t, const char *)
  //

  /**
   * Translate \a host_name and/or \a service_name using specified \a flags.
   * Only entries with \a protocol are returned. On failure, set \a error and
   * return empty result set.
   */
  results_t resolve (const protocol_t &protocol,
    const char *host_name,
    const char *service_name,
    flags_t flags,
    std::error_code &error
  ) noexcept;


  /**
   * Translate \a host_name and/or \a service_name. Only entries with
   * \a protocol are returned. On failure, set \a error and return empty
   * result set.
   */
  results_t resolve (const protocol_t &protocol,
    const char *host_name,
    const char *service_name,
    std::error_code &error) noexcept
  {
    return resolve(protocol, host_name, service_name, flags_t(), error);
  }


  /**
   * Translate \a host_name and/or \a service_name using specified \a flags.
   * Only entries with \a protocol are returned. On failure, throw
   * std::system_error.
   */
  results_t resolve (const protocol_t &protocol,
    const char *host_name,
    const char *service_name,
    flags_t flags)
  {
    return resolve(protocol,
      host_name,
      service_name,
      flags,
      throw_on_error("resolve")
    );
  }


  /**
   * Translate \a host_name and/or \a service_name. Only entries with
   * \a protocol are returned. On failure, throw std::system_error.
   */
  results_t resolve (const protocol_t &protocol,
    const char *host_name,
    const char *service_name)
  {
    return resolve(protocol, host_name, service_name, flags_t());
  }


private:

  const int socktype_ = endpoint_t().protocol().type();
  const int protocol_ = endpoint_t().protocol().protocol();
};


template <typename Protocol>
basic_resolver_results_t<Protocol> basic_resolver_t<Protocol>::resolve (
  const char *host_name,
  const char *service_name,
  flags_t flags,
  std::error_code &error) noexcept
{
  addrinfo hints, *results{};

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_flags = static_cast<int>(flags);
  hints.ai_socktype = socktype_;
  hints.ai_protocol = protocol_;

  auto error_code = ::getaddrinfo(host_name, service_name, &hints, &results);
  if (error_code)
  {
    error.assign(
      __bits::to_gai_error(error_code, host_name, service_name),
      resolver_category()
    );
  }

  return results_t{host_name, service_name, results};
}


template <typename Protocol>
basic_resolver_results_t<Protocol> basic_resolver_t<Protocol>::resolve (
  const protocol_t &protocol,
  const char *host_name,
  const char *service_name,
  flags_t flags,
  std::error_code &error) noexcept
{
  addrinfo hints, *results{};

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_flags = static_cast<int>(flags);
  hints.ai_family = protocol.family();
  hints.ai_socktype = protocol.type();
  hints.ai_protocol = protocol.protocol();

  auto error_code = ::getaddrinfo(host_name, service_name, &hints, &results);
  if (error_code)
  {
    error.assign(
      __bits::to_gai_error(error_code, host_name, service_name),
      resolver_category()
    );
  }

  return results_t{host_name, service_name, results};
}


} // namespace net::ip


__sal_end
