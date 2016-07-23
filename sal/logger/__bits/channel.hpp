#pragma once

#include <sal/config.hpp>
#include <sal/logger/fwd.hpp>
#include <sal/logger/sink.hpp>
#include <string>


namespace sal { namespace logger {
__sal_begin


namespace __bits {


template <typename T>
struct channel_option_t
{
  T value;

  channel_option_t (const T &value)
    : value(value)
  {}
};


// Common channel data
struct channel_base_t
{
  const std::string name;
  volatile bool is_enabled = true;
  sink_ptr sink = stdout_sink();


  channel_base_t (const std::string &name)
    : name(name)
  {}


  channel_base_t () = delete;
  channel_base_t (const channel_base_t &) = delete;
  channel_base_t (channel_base_t &&) = delete;
  channel_base_t &operator= (const channel_base_t &) = delete;
  channel_base_t &operator= (channel_base_t &&) = delete;


  bool set_option (channel_option_t<sink_ptr> &&option) noexcept
  {
    sink = option.value;
    return false;
  }
};


// Worker-specific channel data
// (stored in map, ref owned by public channel, not inherited)
template <typename Worker>
struct channel_t final
  : public channel_base_t
{
  // ref to Worker that owns this
  Worker &worker;


  template <typename... Options>
  channel_t (const std::string &name, Worker &worker, Options &&...options)
    : channel_base_t(name)
    , worker(worker)
  {
    bool unused[] = { set_option(std::forward<Options>(options))..., false };
    (void)unused;
  }
};


} // namespace __bits


__sal_end
}} // namespace sal::logger
