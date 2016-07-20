#pragma once

#include <sal/config.hpp>
#include <sal/logger/fwd.hpp>
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


// Common channel data
struct channel_base_t
{
  const std::string name;
  volatile bool is_enabled = true;
  sink_ptr sink = default_sink();


  channel_base_t (const std::string &name)
    : name(name)
  {}


  channel_base_t () = delete;
  channel_base_t (const channel_base_t &) = delete;
  channel_base_t (channel_base_t &&) = delete;
  channel_base_t &operator= (const channel_base_t &) = delete;
  channel_base_t &operator= (channel_base_t &&) = delete;


  static sink_ptr default_sink () noexcept;


  bool set_option (const option_t<sink_ptr> &option) noexcept
  {
    sink = option.value;
    return true;
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
    bool unused[] = { channel_base_t::set_option(options)..., false };
    (void)unused;
  }
};


} // namespace __bits


__sal_end
}} // namespace sal::logger