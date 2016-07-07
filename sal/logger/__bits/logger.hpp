#pragma once

#include <sal/config.hpp>
#include <sal/logger/fwd.hpp>
#include <sal/logger/level.hpp>
#include <string>


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


// Common logger data
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


// Worker-specific logger data
// (stored in map, ref owned by public logger, not inherited)
template <typename Worker>
struct logger_t final
  : public logger_base_t
{
  // ref to Worker that owns this
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


} // namespace __bits


__sal_end
}} // namespace sal::logger
