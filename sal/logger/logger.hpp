#pragma once

/**
 * \file sal/logger/logger.hpp
 */


#include <sal/config.hpp>
#include <sal/logger/fwd.hpp>
#include <sal/logger/event.hpp>
#include <sal/logger/level.hpp>
#include <unordered_map>



namespace sal { namespace logger {
__sal_begin


namespace __bits {


template <typename T>
struct option_t
{
  T value;

  option_t (const T &value)
    : value(value)
  {}
};


struct logger_base_t
{
  const std::string name;
  threshold_t threshold = default_threshold();
  sink_ptr sink = default_sink();


  logger_base_t (const std::string &name)
    : name(name)
  {}


  logger_base_t () = delete;
  logger_base_t (const logger_base_t &) = delete;
  logger_base_t (logger_base_t &&) = delete;
  logger_base_t &operator= (const logger_base_t &) = delete;
  logger_base_t &operator= (logger_base_t &&) = delete;


  static level_t default_threshold () noexcept;
  static sink_ptr default_sink () noexcept;


  bool set_option (const option_t<threshold_t> &option) noexcept
  {
    threshold = option.value;
    return true;
  }


  bool set_option (const option_t<sink_ptr> &option) noexcept
  {
    sink = option.value;
    return true;
  }
};


template <typename Worker>
struct logger_t
  : public logger_base_t
{
  Worker &worker;


  template <typename... Options>
  logger_t (const std::string &name, Worker &worker, Options &&...options)
    : logger_base_t(name)
    , worker(worker)
  {
    bool unused[] = { logger_base_t::set_option(options)..., false };
    (void)unused;
  }
};


template <typename Worker>
struct worker_t
{
  std::unordered_map<std::string, logger_t<Worker>> loggers{};
  const logger_t<Worker> &default_logger;


  template <typename... Options>
  worker_t (Worker &worker, Options &&...options)
    : default_logger(
        loggers.emplace(std::piecewise_construct,
          std::forward_as_tuple(""),
          std::forward_as_tuple("",
            worker,
            std::forward<Options>(options)...
          )
        ).first->second
      )
  {}


  template <typename... Options>
  const logger_t<Worker> &make_logger (const std::string &name,
    Options &&...options)
  {
    return loggers.emplace(std::piecewise_construct,
      std::forward_as_tuple(name),
      std::forward_as_tuple(name,
        default_logger.worker,
        default_logger.threshold,
        default_logger.sink,
        std::forward<Options>(options)...
      )
    ).first->second;
  }


  worker_t () = delete;
  worker_t (const worker_t &) = delete;
  worker_t (worker_t &&) = delete;
  worker_t &operator= (const worker_t &) = delete;
  worker_t &operator= (worker_t &&) = delete;
};


} // namespace __bits


template <typename Worker>
class logger_t
{
public:

  const std::string &name () const noexcept
  {
    return impl_.name;
  }


  bool is_enabled (level_t level) const noexcept
  {
    return impl_.threshold.is_enabled(level);
  }


  event_ptr make_event (level_t level) const noexcept
  {
    return event_ptr{
      impl_.worker.alloc_and_init(level, *this),
      &Worker::write_and_release
    };
  }


private:

  using impl_t = __bits::logger_t<Worker>;
  const impl_t &impl_;

  logger_t (const impl_t &impl)
    : impl_(impl)
  {}

  friend class basic_worker_t<Worker>;
  friend Worker;
};


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
  return __bits::option_t<sink_ptr>(sink);
}


template <typename Worker>
class basic_worker_t
{
public:

  using logger_type = logger_t<Worker>;


  template <typename... Options>
  basic_worker_t (Worker &worker, Options &&...options)
    : impl_(worker, std::forward<Options>(options)...)
  {}


  logger_type default_logger () const noexcept
  {
    return impl_.default_logger;
  }


  logger_type get_logger (const std::string &name) const noexcept
  {
    auto it = impl_.loggers.find(name);
    return it != impl_.loggers.end() ? it->second : impl_.default_logger;
  }


  template <typename... Options>
  logger_type make_logger (const std::string &name, Options &&...options)
  {
    return impl_.make_logger(name, std::forward<Options>(options)...);
  }


private:

  __bits::worker_t<Worker> impl_;
};


class worker_t
  : public basic_worker_t<worker_t>
{
public:

  template <typename... Options>
  worker_t (Options &&...options)
    : basic_worker_t(*this, std::forward<Options>(options)...)
  {}


private:

  event_t *alloc_and_init (level_t level, const logger_type &logger) noexcept
  {
    if (auto event = new (std::nothrow) event_t)
    {
      event->message.reset();
      event->level = level;
      event->time = now();
      event->sink = logger.impl_.sink.get();
      event->logger_name = &logger.name();
      // TODO event->sink->init(*event);
      return event;
    }
    return nullptr;
  }

  static void write_and_release (event_t *event) noexcept
  {
    // TODO event->sink->write(*event);
    delete event;
  }

  friend class logger_t<worker_t>;
};


__sal_end
}} // namespace sal::logger
