#pragma once

/**
 * \file sal/logger/worker.hpp
 * Logging channels' worker, marshalling event records from channels to sinks.
 *
 * \addtogroup logger
 * \{
 */


#include <sal/config.hpp>
#include <sal/logger/__bits/channel.hpp>
#include <sal/logger/channel.hpp>
#include <sal/assert.hpp>
#include <unordered_map>


namespace sal { namespace logger {
__sal_begin


/**
 * Return option to configure channel's sink.
 */
inline auto set_sink (const sink_ptr &sink) noexcept
{
  sal_check_ptr(sink.get());
  return __bits::option_t<sink_ptr>(sink);
}


/**
 * Base class for different worker implementations. This class provides
 * functionality to create, configure and query channels. Each channel is
 * identified by name (e.g. module names that use it to log events etc).
 * Worker remains owner of channel throughout whole lifetime.
 *
 * Each worker owns default channel that is created and configured during
 * worker construction. It's attributes are used as defaults when adding
 * new channels. Also, when querying for non-existing channel, default one
 * is returned.
 *
 * Inherited classes should provide two methods that are used by channel to
 * handle events:
 *   - event_t *alloc_and_init (const channel_t<Worker> &channel)
 *     Create new event (or fetch from pool) and initialize it's members.
 *     Return raw pointer to channel that will wrap it into unique_ptr,
 *     specifiying write_and_release() as custom deleter.
 *   - static void write_and_release (event_t &event);
 *     After application layer has filled event message, this method delivers
 *     event to final destination (sink) and after that releases event (or
 *     returns to pool)
 *
 * \see worker_t
 * \see async_worker_t
 *
 * \note After creation, channel remains immutable (except enabled/disabled
 * flag) until worker is destructed. This approach prevents threading issues.
 */
template <typename Worker>
class basic_worker_t
{
public:

  /// Channel type for specified \a Worker
  using channel_type = channel_t<Worker>;


  /**
   * Construct worker: create and configure default channel using \a options
   *
   * \note Specified options become default values for new channels if their
   * own options are not set or set partially.
   *
   * \see set_sink
   */
  template <typename... Options>
  basic_worker_t (Options &&...options)
    : default_channel_(
        channels_.emplace(std::piecewise_construct,
          std::forward_as_tuple(""),
          std::forward_as_tuple("",
            static_cast<Worker &>(*this),
            std::forward<Options>(options)...
          )
        ).first->second
      )
  {}


  /**
   * Return reference to default channel created during worker construction
   */
  channel_type default_channel () noexcept
  {
    return default_channel_;
  }


  /**
   * Return previously created channel with \a name or default channel if not
   * found.
   */
  channel_type get_channel (const std::string &name) noexcept
  {
    auto it = channels_.find(name);
    return it != channels_.end() ? it->second : default_channel_;
  }


  /**
   * Create new channel \a with name, using \a options. If some or none of
   * options are specified, corresponding defaults are taken from
   * default_channel()
   *
   * \see set_sink
   */
  template <typename... Options>
  channel_type make_channel (const std::string &name, Options &&...options)
  {
    return channels_.emplace(std::piecewise_construct,
      std::forward_as_tuple(name),
      std::forward_as_tuple(name,
        static_cast<Worker &>(*this),
        default_channel_.sink,
        std::forward<Options>(options)...
      )
    ).first->second;
  }


  /**
   * Visit each channel and set it's \a enabled state if \a filter predicate
   * returns true. Signature of the \a filter should be equivalent of:
   * \code{.cpp}
   * bool filter (const std::string &channel_name);
   * \endcode
   *
   * This method help to turn on/off group of channels. Example, turn off all
   * channels that has suffix '.debug' in their name:
   * \code
   * worker.set_enabled_if(false,
   *   [](auto channel_name)
   *   {
   *     return ends_with(channel_name, ".debug");
   *   }
   * )
   * \endcode
   */
  template <typename Filter>
  void set_enabled_if (bool enabled, Filter &&filter)
  {
    for (auto &channel: channels_)
    {
      if (std::forward<Filter>(filter)(channel.second.name))
      {
        channel.second.is_enabled = enabled;
      }
    }
  }


private:

  std::unordered_map<std::string, __bits::channel_t<Worker>> channels_{};
  __bits::channel_t<Worker> &default_channel_;
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


  /**
   * Create global default logging worker with \a options. This worker is used
   * by logging macros that do not specify channel explicitly.
   *
   * \note This method can be called only once.
   */
  template <typename... Options>
  static worker_t &make_default (Options &&...options)
  {
    sal_assert(default_ == nullptr);
    default_ = std::make_unique<worker_t>(std::forward<Options>(options)...);
    return *sal_check_ptr(default_.get());
  }


  /**
   * Return global default logger worker. If not created yet, it will
   * internally call worker_t::make_default() with no arguments i.e. using
   * default settings.
   */
  static worker_t &get_default ()
  {
    static auto &worker_(default_ ? *default_ : make_default());
    return worker_;
  }


private:

  event_t *alloc_and_init (const channel_type &channel);
  static void write_and_release (event_t *event);

  static std::unique_ptr<worker_t> default_;

  friend class channel_t<worker_t>;
};


/**
 * Create global default logging worker with \a options
 * \see worker_t::make_default()
 */
template <typename... Options>
inline worker_t &make_default_worker (Options &&...options)
{
  return worker_t::make_default(std::forward<Options>(options)...);
}


/**
 * Return global default logger worker.
 * \see worker_t::get_default()
 */
inline worker_t &default_worker ()
{
  return worker_t::get_default();
}


/**
 * Return global default channel for worker_t::get_default() worker.
 */
inline channel_t<worker_t> &default_channel ()
{
  static auto channel_(default_worker().default_channel());
  return channel_;
}


__sal_end
}} // namespace sal::logger

/// \}
