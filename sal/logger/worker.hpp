#pragma once

/**
 * \file sal/logger/worker.hpp
 */


#include <sal/config.hpp>
#include <sal/logger/__bits/logger.hpp>
#include <sal/logger/logger.hpp>
#include <sal/assert.hpp>
#include <unordered_map>


namespace sal { namespace logger {
__sal_begin


/**
 * Return option to configure logger's threshold.
 */
inline auto set_threshold (level_t level) noexcept
{
  return __bits::option_t<threshold_t>(level);
}


/**
 * Return option to configure logger's sink.
 */
inline auto set_sink (const sink_ptr &sink) noexcept
{
  sal_check_ptr(sink.get());
  return __bits::option_t<sink_ptr>(sink);
}


/**
 * Base class for different worker implementations. This class provides
 * functionality to create, configure and query loggers. Each logger is
 * identified by name (e.g. module names that use it to log events etc).
 * Worker remains owner throughout whole lifetime.
 *
 * Each worker owns default logger that is created and configured during
 * worker construction. It's attributes are used as defaults when adding
 * new loggers. Also, when querying for non-existing logger, default one
 * is returned.
 *
 * Inherited classes should provide two methods that are used by logger to
 * handle events:
 *   - event_t *alloc_and_init (level_t level, const logger_t<Worker> &logger)
 *     Create new event (or fetch from pool) and initialize it's members.
 *     Return raw pointer to logger that will wrap it into unique_ptr,
 *     specifiying write_and_release() as custom deleter.
 *   - static void write_and_release (event_t &event);
 *     After application layer has filled event message, this method delivers
 *     event to final destination (sink) and after that releases event (or
 *     returns to pool)
 *
 * \see worker_t
 * \see async_worker_t
 *
 * \note After creation, logger remains unmutable until worker itself is
 * destructed. This approach prevents threading issues.
 */
template <typename Worker>
class basic_worker_t
{
public:

  /// Logger type for specified \a Worker
  using logger_type = logger_t<Worker>;


  /**
   * Construct worker: create and configure default logger using \a options
   *
   * \note Specified options become default values for new loggers if their
   * own options are not set or set partially.
   *
   * \see set_threshold
   * \see set_sink
   */
  template <typename... Options>
  basic_worker_t (Options &&...options)
    : default_logger_(
        loggers_.emplace(std::piecewise_construct,
          std::forward_as_tuple(""),
          std::forward_as_tuple("",
            static_cast<Worker &>(*this),
            std::forward<Options>(options)...
          )
        ).first->second
      )
  {}


  /**
   * Return reference to default logger created during worker construction
   */
  logger_type default_logger () const noexcept
  {
    return default_logger_;
  }


  /**
   * Return previously created logger with \a name or default logger if not
   * found.
   */
  logger_type get_logger (const std::string &name) const noexcept
  {
    auto it = loggers_.find(name);
    return it != loggers_.end() ? it->second : default_logger_;
  }


  /**
   * Create new logger \a with name, using \a options. If some or none of
   * options are specified, corresponding defaults are taken from
   * default_logger()
   *
   * \see set_threshold
   * \see set_sink
   */
  template <typename... Options>
  logger_type make_logger (const std::string &name, Options &&...options)
  {
    return loggers_.emplace(std::piecewise_construct,
      std::forward_as_tuple(name),
      std::forward_as_tuple(name,
        static_cast<Worker &>(*this),
        default_logger_.threshold,
        default_logger_.sink,
        std::forward<Options>(options)...
      )
    ).first->second;
  }


private:

  std::unordered_map<std::string, __bits::logger_t<Worker>> loggers_{};
  const __bits::logger_t<Worker> &default_logger_;
};


/**
 * Simple synchronous logger. It is direct descendant of basic_worker_t that
 * holds single event per thread that is used for logging. It implies that
 * application's each thread have single event logging in progress at any
 * time. It is ok for multiple threads doing logging same time.
 *
 * While simple and doesn't do any memory allocations, any thread can block
 * if event message writing blocks. If such blocking is undesirable, it might
 * be better to use async_worker_t.
 */
class worker_t
  : public basic_worker_t<worker_t>
{
public:

  /// Construct worker, passing \a options to basic_worker_t<worker_t>
  template <typename... Options>
  worker_t (Options &&...options)
    : basic_worker_t(std::forward<Options>(options)...)
  {}


private:

  event_t *alloc_and_init (level_t level, const logger_type &logger);
  static void write_and_release (event_t *event);

  friend class logger_t<worker_t>;
};


__sal_end
}} // namespace sal::logger
