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


  constexpr basic_resolver_entry_t () = default;


  constexpr const endpoint_t &endpoint () const noexcept
  {
    return endpoint_;
  }


  constexpr operator endpoint_t () const noexcept
  {
    return endpoint();
  }


  const char *host_name () const noexcept
  {
    return host_name_;
  }


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


template <typename Protocol>
class basic_resolver_results_iterator_t
{
public:

  constexpr basic_resolver_results_iterator_t () = default;


  const basic_resolver_entry_t<Protocol> &operator* () const noexcept
  {
    return entry_;
  }


  const basic_resolver_entry_t<Protocol> *operator-> () const noexcept
  {
    return &entry_;
  }


  basic_resolver_results_iterator_t &operator++ ()
  {
    it_ = it_->ai_next;
    entry_.load(it_);
    return *this;
  }


  basic_resolver_results_iterator_t operator++ (int)
  {
    auto tmp(*this);
    ++*this;
    return tmp;
  }


  friend bool operator== (const basic_resolver_results_iterator_t &a,
    const basic_resolver_results_iterator_t &b) noexcept
  {
    return a.it_ == b.it_;
  }


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


template <typename Protocol>
class basic_resolver_results_t
{
public:

  using protocol_t = Protocol;
  using endpoint_t = typename Protocol::endpoint_t;

  using value_t = basic_resolver_entry_t<Protocol>;
  using const_reference = const value_t &;
  using reference = value_t &;

  using const_iterator = basic_resolver_results_iterator_t<Protocol>;
  using iterator = const_iterator;


  constexpr basic_resolver_results_t () = default;


  ~basic_resolver_results_t () noexcept
  {
    if (results_)
    {
      ::freeaddrinfo(results_);
    }
  }


  basic_resolver_results_t (basic_resolver_results_t &&that) noexcept
    : host_name_(std::move(that.host_name_))
    , service_name_(std::move(that.service_name_))
    , results_(that.results_)
    , size_(that.size_)
  {
    that.results_ = nullptr;
    that.size_ = 0;
  }


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


  const std::string &host_name () const noexcept
  {
    return host_name_;
  }


  const std::string &service_name () const noexcept
  {
    return service_name_;
  }


  size_t size () const noexcept
  {
    return size_;
  }


  bool empty () const noexcept
  {
    return size_ == 0;
  }


  const_iterator begin () const noexcept
  {
    return const_iterator(results_,
      host_name_.c_str(),
      service_name_.c_str()
    );
  }


  const_iterator end () const noexcept
  {
    return const_iterator();
  }


  const_iterator cbegin () const noexcept
  {
    return const_iterator(results_,
      host_name_.c_str(),
      service_name_.c_str()
    );
  }


  const_iterator cend () const noexcept
  {
    return const_iterator();
  }


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


class resolver_base_t
{
public:

  using flags_t = int;
  static constexpr flags_t passive = AI_PASSIVE;
  static constexpr flags_t canonical_name = AI_CANONNAME;
  static constexpr flags_t numeric_host = AI_NUMERICHOST;
  static constexpr flags_t numeric_service = AI_NUMERICSERV;
  static constexpr flags_t v4_mapped = AI_V4MAPPED;
  static constexpr flags_t all_matching = AI_ALL;
  static constexpr flags_t address_configured = AI_ADDRCONFIG;

protected:

  ~resolver_base_t () noexcept
  {}
};


template <typename Protocol>
class basic_resolver_t
  : public resolver_base_t
{
public:

  using protocol_t = Protocol;
  using endpoint_t = typename Protocol::endpoint_t;
  using results_t = basic_resolver_results_t<Protocol>;


  //
  // resolve (const char *)
  //

  results_t resolve (const char *host_name,
    const char *service_name,
    flags_t flags,
    std::error_code &error
  ) noexcept;


  results_t resolve (const char *host_name,
    const char *service_name,
    std::error_code &error) noexcept
  {
    return resolve(host_name, service_name, flags_t(), error);
  }


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


  results_t resolve (const char *host_name,
    const char *service_name)
  {
    return resolve(host_name, service_name, flags_t());
  }


  //
  // resolve (protocol_t, const char *)
  //

  results_t resolve (const protocol_t &protocol,
    const char *host_name,
    const char *service_name,
    flags_t flags,
    std::error_code &error
  ) noexcept;


  results_t resolve (const protocol_t &protocol,
    const char *host_name,
    const char *service_name,
    std::error_code &error) noexcept
  {
    return resolve(protocol, host_name, service_name, flags_t(), error);
  }


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


  results_t resolve (const protocol_t &protocol,
    const char *host_name,
    const char *service_name)
  {
    return resolve(protocol, host_name, service_name, flags_t());
  }


  //
  // resolve (endpoint_t)
  //

  results_t resolve (const endpoint_t &endpoint, std::error_code &error)
    noexcept;


  results_t resolve (const endpoint_t &endpoint)
  {
    std::error_code error;
    auto result = resolve(endpoint, error);
    if (!error)
    {
      return result;
    }
    throw_system_error(error, "resolve");
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
