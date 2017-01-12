#pragma once

/**
 * \file sal/net/ip/basic_resolver_entry.hpp
 * Internet endpoint resolver result set entry
 */


#include <sal/config.hpp>
#include <sal/net/ip/basic_endpoint.hpp>


__sal_begin


namespace net { namespace ip {


/**
 * Resolver result entry
 */
template <typename Protocol>
class basic_resolver_entry_t
{
public:

  /**
   * Protocol type
   */
  using protocol_t = Protocol;

  /**
   * Protocol's endpoint type
   */
  using endpoint_t = typename Protocol::endpoint_t;


  /**
   * Create entry with default endpoint and empty host_name() and
   * service_name().
   */
  constexpr basic_resolver_entry_t () = default;


  /**
   * Return endpoint associated with resolver entry.
   */
  constexpr const endpoint_t &endpoint () const noexcept
  {
    return endpoint_;
  }


  /**
   * \copydoc endpoint()
   */
  constexpr operator endpoint_t () const noexcept
  {
    return endpoint();
  }


  /**
   * Return host name associated with resolver entry
   */
  const char *host_name () const noexcept
  {
    return host_name_;
  }


  /**
   * Return service name associated with resolver entry
   */
  const char *service_name () const noexcept
  {
    return service_name_;
  }


private:

  endpoint_t endpoint_{};
  const char *host_name_{}, *service_name_{};

  basic_resolver_entry_t (const char *host_name, const char *service_name)
      noexcept
    : host_name_(host_name)
    , service_name_(service_name)
  {}

  void load (const addrinfo *ai) noexcept
  {
    if (ai)
    {
      endpoint_.try_load(*reinterpret_cast<const sockaddr_storage *>(ai->ai_addr));
      if (ai->ai_canonname)
      {
        host_name_ = ai->ai_canonname;
      }
    }
    else
    {
      endpoint_ = endpoint_t();
    }
  }

  friend class basic_resolver_results_iterator_t<Protocol>;
};


}} // namespace net::ip


__sal_end
