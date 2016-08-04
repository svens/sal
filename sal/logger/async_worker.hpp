#pragma once

/**
 * \file sal/logger/async_worker.hpp
 * \todo docs
 *
 * \addtogroup logger
 * \{
 */

#include <sal/config.hpp>
#include <sal/logger/worker.hpp>


namespace sal { namespace logger {
__sal_begin


/**
 * Asynchronous logger worker.
 */
class async_worker_t
  : public basic_worker_t<async_worker_t>
{
public:

  /// Construct worker, passing \a options to basic_worker_t<async_worker_t>
  template <typename... Options>
  async_worker_t (Options &&...options)
    : basic_worker_t(std::forward<Options>(options)...)
  {}


private:

  event_ptr make_event (const channel_type &channel) noexcept;
  friend class channel_t<async_worker_t>;
};


__sal_end
}} // namespace sal::logger

/// \}
