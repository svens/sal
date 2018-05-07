#pragma once

/**
 * \file sal/crypto/channel_options.hpp
 * Secure encrypted channel options.
 */


#include <sal/config.hpp>
#include <sal/crypto/certificate.hpp>
#include <functional>
#include <string>


__sal_begin


namespace crypto {


/**
 * \defgroup channel_options Channel options
 * \{
 */


/**
 * Base class for channel options.
 */
template <typename Option>
struct channel_option_t
{};


/**
 * Mutual authentication option class.
 * \see mutual_auth
 * \see no_mutual_auth
 */
struct mutual_auth_t
  : public channel_option_t<mutual_auth_t>
{
  /**
   * True if mutual authentication is required.
   */
  const bool value;

  /**
   * \a require mutual authentication.
   */
  constexpr mutual_auth_t (bool require) noexcept
    : value(require)
  { }
};


/**
 * Require mutual authentication.
 */
constexpr mutual_auth_t mutual_auth{true};

/**
 * Do not require mutual authentication.
 */
constexpr mutual_auth_t no_mutual_auth{false};


/**
 * Remote peer name to check when connecting (or accepting with mutual
 * authentication requirement)
 *
 * \see peer_name()
 */
struct peer_name_t
  : public channel_option_t<peer_name_t>
{
  /**
   * Peer name.
   */
  const std::string value;

  /**
   * Construct instance of \a peer_name_t using \a peer_name.
   */
  peer_name_t (std::string peer_name)
    : value(peer_name)
  {}
};


/**
 * Create peer \a name channel option instance.
 */
inline peer_name_t peer_name (std::string name)
{
  return {name};
}


/// \}


/**
 * \defgroup channel_factory_options Channel factory options
 */

/**
 * Base class for channel factory options.
 */
template <typename Option>
struct channel_factory_option_t
{};


/**
 * Channel factory option for configuring used certificate chain.
 * \see with_chain()
 * \see with_certificate()
 */
struct with_chain_t
  : public channel_factory_option_t<with_chain_t>
{
  /**
   * List of certificates to present to peer. First item must have
   * corresponding private key that is used to secure new channels.
   */
  std::vector<certificate_t> value;

  /**
   * Construct instance of \a with_chain_t using \a certificates
   */
  with_chain_t (std::vector<certificate_t> certificates) noexcept
    : value(certificates)
  {}
};


/**
 * Use single \a certificate during handshake.
 */
inline with_chain_t with_certificate (const certificate_t &certificate)
  noexcept
{
  return {{certificate}};
}


/**
 * Use \a chain during handshake.
 */
inline with_chain_t with_chain (std::vector<certificate_t> chain)
  noexcept
{
  return chain;
}


/**
 * Private key to secure new channels. On platforms where private key is kept
 * in secure store (MacOS, Windows), this option is non-functional.
 */
struct with_private_key_t
  : public channel_factory_option_t<with_private_key_t>
{
  /**
   * Pointer to private key to use.
   */
  const private_key_t * const private_key;

  /**
   * Construct new \a private_key option instance.
   */
  with_private_key_t (const private_key_t *private_key) noexcept
    : private_key(private_key)
  {}
};


/**
 * Return private key channel factory option with \a private_key
 */
inline with_private_key_t with_private_key (const private_key_t *private_key)
  noexcept
{
  return {private_key};
}


/**
 * Channel factory option to switch to manual certificate chain check instead
 * of OS's own check.
 */
template <typename Check>
struct chain_check_t
  : public channel_factory_option_t<chain_check_t<Check>>
{
  /**
   * Function that checks if \a chain can be trusted. Returns true if yes and
   * false otherwise.
   */
  std::function<bool(const std::vector<crypto::certificate_t> &chain)> value{};

  /**
   * Construct channel factory option to check peer chain.
   */
  chain_check_t (Check check) noexcept
    : value(check)
  {}
};


/**
 * Return instance of channel factory option to \a check peer's chain.
 */
template <typename Check>
inline chain_check_t<Check> chain_check (Check check)
  noexcept
{
  return {check};
}


/**
 * No-op peer chain checker that trusts any presented certificate chain.
 * \warning Do not use it unless testing.
 */
inline const auto no_chain_check = chain_check(
  [](const std::vector<certificate_t> &) noexcept
  {
    return true;
  }
);


/// \}


} // namespace crypto


__sal_end
