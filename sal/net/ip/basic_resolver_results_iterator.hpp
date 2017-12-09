#pragma once

/**
 * \file sal/net/ip/basic_resolver_results_iterator.hpp
 * Internet endpoint resolver result set iterator
 */


#include <sal/config.hpp>
#include <sal/net/ip/basic_resolver_entry.hpp>


__sal_begin


namespace net { namespace ip {


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
  basic_resolver_entry_t<Protocol> entry_{};

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


}} // namespace net::ip


__sal_end
