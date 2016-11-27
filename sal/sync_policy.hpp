#pragma once

/**
 * \file sal/sync_policy.hpp
 * Data structures synchronised access policies
 */

#include <sal/config.hpp>


__sal_begin


/**
 * Unsynchronised access policy
 */
struct no_sync_t
{
  /**
   * Intrusive_queue_t hook.
   * Opaque data, not for application layer use
   */
  using intrusive_queue_hook_t = void *;
};


/**
 * Single-producer, single-consumer access policy
 */
struct spsc_sync_t
{
  /**
   * Intrusive_queue_t hook.
   * Opaque data, not for application layer use
   */
  struct intrusive_queue_hook_t
  {
    volatile void *next;        ///< Opaque data
    volatile unsigned seq;      ///< Opaque data
  };
};


/**
 * Multi-producer, single-consumer access policy
 */
struct mpsc_sync_t
{
  /**
   * Intrusive_queue_t hook.
   * Opaque data, not for application layer use
   */
  using intrusive_queue_hook_t = volatile void *;
};


namespace __bits {

constexpr size_t hardware_destructive_interference_size ()
{
  /// \todo std::hardware_destructive_interference_size
  return 64;
}

} // namespace __bits


__sal_end
