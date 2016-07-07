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


template <typename Worker>
class basic_worker_t
{
public:

  using logger_type = logger_t<Worker>;


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


  logger_type default_logger () const noexcept
  {
    return default_logger_;
  }


  logger_type get_logger (const std::string &name) const noexcept
  {
    auto it = loggers_.find(name);
    return it != loggers_.end() ? it->second : default_logger_;
  }


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


class worker_t
  : public basic_worker_t<worker_t>
{
public:

  template <typename... Options>
  worker_t (Options &&...options)
    : basic_worker_t(std::forward<Options>(options)...)
  {}


private:

  event_t *alloc_and_init (level_t level, const logger_type &logger) noexcept;
  static void write_and_release (event_t *event) noexcept;

  friend class logger_t<worker_t>;
};


__sal_end
}} // namespace sal::logger
