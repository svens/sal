#pragma once

/**
 * \file sal/net/ip/basic_resolver_results.hpp
 * Internet endpoint resolver result set
 */


#include <sal/config.hpp>
#include <sal/net/ip/basic_resolver_entry.hpp>
#include <sal/net/ip/basic_resolver_results_iterator.hpp>
#include <string>


__sal_begin


namespace net { namespace ip {


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


  basic_resolver_results_t (const basic_resolver_results_t &) = delete;
  basic_resolver_results_t &operator= (const basic_resolver_results_t &) = delete;


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

  std::string host_name_{}, service_name_{};
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


}} // namespace net::ip


__sal_end
