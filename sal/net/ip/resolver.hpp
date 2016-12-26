#pragma once

/**
 * \file sal/net/ip/resolver.hpp
 * Internet endpoint resolver
 */


#include <sal/config.hpp>
#include <sal/net/fwd.hpp>
#include <sal/net/error.hpp>


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


/**
 * Resolver result entries iterator. All operations are well-defined only if
 * iterator points to valid result entry.
 */
template <typename Protocol>
class basic_resolver_results_iterator_t
{
public:

  /**
   * Create iterator that is not associated with any result.
   */
  constexpr basic_resolver_results_iterator_t () = default;


  /**
   * Return reference to current entry iterator points to.
   */
  const basic_resolver_entry_t<Protocol> &operator* () const noexcept
  {
    return entry_;
  }


  /**
   * Return address to current entry iterator points to.
   */
  const basic_resolver_entry_t<Protocol> *operator-> () const noexcept
  {
    return &entry_;
  }


  /**
   * Iterator pre-increment. Returns reference to self after moving iterator
   * to next entry.
   */
  basic_resolver_results_iterator_t &operator++ ()
  {
    it_ = it_->ai_next;
    entry_.load(it_);
    return *this;
  }


  /**
   * Iterator post-increment. Returns reference to old entry prior moving
   * itertor to next entry.
   */
  basic_resolver_results_iterator_t operator++ (int)
  {
    auto tmp(*this);
    ++*this;
    return tmp;
  }


  /**
   * Returns true if both \a a and \a b point to same entry.
   */
  friend bool operator== (const basic_resolver_results_iterator_t &a,
    const basic_resolver_results_iterator_t &b) noexcept
  {
    return a.it_ == b.it_;
  }


  /**
   * Returns true if \a a and \a b point to different entries.
   */
  friend bool operator!= (const basic_resolver_results_iterator_t &a,
    const basic_resolver_results_iterator_t &b) noexcept
  {
    return !(a == b);
  }


private:

  addrinfo *it_{};
  basic_resolver_entry_t<Protocol> entry_;

  basic_resolver_results_iterator_t (addrinfo *it,
      const char *host_name,
      char const *service_name)
    : it_(it)
    , entry_{host_name, service_name}
  {
    entry_.load(it_);
  }

  friend class basic_resolver_results_t<Protocol>;
};


/**
 * Sequence of basic_resolver_entry_t<Protocol> elements resulting from single
 * name resolution operation.
 */
template <typename Protocol>
class basic_resolver_results_t
{
public:

  /// Protocol type
  using protocol_t = Protocol;

  /// Protocol's endpoint type
  using endpoint_t = typename Protocol::endpoint_t;

  /// Single entry for resolve() operation
  using value_t = basic_resolver_entry_t<Protocol>;

  /// const reference to \a value_t
  using const_reference = const value_t &;

  /// mutable reference to \a value_t
  using reference = value_t &;

  /// const iterator over resolve operation result set
  using const_iterator = basic_resolver_results_iterator_t<Protocol>;

  /// iterator over resolve operation result set
  using iterator = const_iterator;


  /// Construct empty resolver results set
  constexpr basic_resolver_results_t () = default;


  /// Destruct resolver result set and release used resources (if not empty)
  ~basic_resolver_results_t () noexcept
  {
    if (results_)
    {
      ::freeaddrinfo(results_);
    }
  }


  /**
   * Move content of \a that into \a this. It is undefined behaviour to use
   * \a that after move (until it has acquired result from new resolve
   * operation or from another result set).
   */
  basic_resolver_results_t (basic_resolver_results_t &&that) noexcept
    : host_name_(std::move(that.host_name_))
    , service_name_(std::move(that.service_name_))
    , results_(that.results_)
    , size_(that.size_)
  {
    that.results_ = nullptr;
    that.size_ = 0;
  }


  /**
   * Move content of \a that into \a this. It is undefined behaviour to use
   * \a that after move (until it has acquired result from new resolve
   * operation or from another result set).
   */
  basic_resolver_results_t &operator= (basic_resolver_results_t &&that)
    noexcept
  {
    host_name_ = std::move(that.host_name_);
    service_name_ = std::move(that.service_name_);
    results_ = that.results_;
    that.results_ = nullptr;
    size_ = that.size_;
    that.size_ = 0;
    return *this;
  }


  /**
   * Return host name associated with resolver query.
   */
  const std::string &host_name () const noexcept
  {
    return host_name_;
  }


  /**
   * Return service name associated with resolver query.
   */
  const std::string &service_name () const noexcept
  {
    return service_name_;
  }


  /**
   * Return number of entries in result set
   */
  size_t size () const noexcept
  {
    return size_;
  }


  /**
   * Return true if result set is empty.
   */
  bool empty () const noexcept
  {
    return size_ == 0;
  }


  /**
   * Return iterator to first entry in result set
   */
  const_iterator begin () const noexcept
  {
    return const_iterator(results_,
      host_name_.c_str(),
      service_name_.c_str()
    );
  }


  /**
   * Return iterator to past end of entries in result set.
   */
  const_iterator end () const noexcept
  {
    return const_iterator();
  }


  /**
   * Return iterator to first entry in result set
   */
  const_iterator cbegin () const noexcept
  {
    return const_iterator(results_,
      host_name_.c_str(),
      service_name_.c_str()
    );
  }


  /**
   * Return iterator to past end of entries in result set.
   */
  const_iterator cend () const noexcept
  {
    return const_iterator();
  }


  /**
   * Swap content of \a this with \a that.
   */
  void swap (basic_resolver_results_t &that) noexcept
  {
    using std::swap;
    swap(host_name_, that.host_name_);
    swap(service_name_, that.service_name_);
    swap(results_, that.results_);
    swap(size_, that.size_);
  }


private:

  std::string host_name_, service_name_;
  addrinfo *results_{};
  size_t size_{};

  basic_resolver_results_t (const char *host_name,
      const char *service_name,
      addrinfo *results) noexcept
    : host_name_(host_name ? host_name : "")
    , service_name_(service_name ? service_name : "")
    , results_(results)
  {
    for (size_ = 0;  results;  results = results->ai_next)
    {
      size_++;
    }
  }

  friend class basic_resolver_t<Protocol>;
};


/**
 * Common ancestor for different resolver types.
 */
class resolver_base_t
{
public:

  /// Resolver query parameters
  using flags_t = int;

  /**
   * POSIX macro AI_PASSIVE. Returned endpoints are intended for use as
   * locally bound socket endpoints.
   */
  static constexpr flags_t passive = AI_PASSIVE;

  /**
   * POSIX macro AI_CANONNAME. Determine the canonical name of the host
   * specified in query.
   */
  static constexpr flags_t canonical_name = AI_CANONNAME;

  /**
   * POSIX macro AI_NUMERICHOST. Host name should be treated as numeric string
   * defining and IPv4 or IPv6 address and no host name resolution should be
   * attempted.
   */
  static constexpr flags_t numeric_host = AI_NUMERICHOST;

  /**
   * POSIX macro AI_NUMERICSERV. Service name should be treated as a numeric
   * string defining a port numebr and no service name resolution should be
   * attempted.
   */
  static constexpr flags_t numeric_service = AI_NUMERICSERV;

  /**
   * POSIX macro AI_V4MAPPED. If the protocol is specified as an IPv6
   * protocol, return IPv4-mapped IPv6 address on finding no IPv6 address.
   */
  static constexpr flags_t v4_mapped = AI_V4MAPPED;

  /**
   * POSIX macro AI_ALL. If used with v4_mapped, return all matching IPv6 and
   * IPv4 addresses.
   */
  static constexpr flags_t all_matching = AI_ALL;

  /**
   * POSIX macro AI_ADDRCONFIG. Only return IPv4 addresses if a non-loopback
   * IPv4 address is configured for the system. Only return IPv4 addresses if
   * a non-loopback IPv4 address is configured for the system.
   */
  static constexpr flags_t address_configured = AI_ADDRCONFIG;


protected:

  ~resolver_base_t () noexcept
  {}
};


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
    std::error_code error;
    auto result = resolve(host_name, service_name, flags, error);
    if (!error)
    {
      return result;
    }
    throw_system_error(error, "resolve");
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
    std::error_code error;
    auto result = resolve(protocol, host_name, service_name, flags, error);
    if (!error)
    {
      return result;
    }
    throw_system_error(error, "resolve");
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


}} // namespace net::ip


__sal_end
