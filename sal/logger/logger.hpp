#pragma once

/**
 * \file sal/logger/logger.hpp
 */


#include <sal/config.hpp>
#include <sal/logger/__bits/logger.hpp>



namespace sal { namespace logger {
__sal_begin


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


__sal_end
}} // namespace sal::logger
