#pragma once

/**
 * \file sal/logger/async_worker.hpp
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

  event_t *alloc_and_init (level_t level, const logger_type &logger) noexcept;
  static void write_and_release (event_t *event) noexcept;

  friend class logger_t<async_worker_t>;
};


__sal_end
}} // namespace sal::logger
