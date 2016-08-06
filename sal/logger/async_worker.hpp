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
#include <memory>


namespace sal { namespace logger {
__sal_begin


/**
 * Asynchronous logger worker.
 */
class async_worker_t
  : public basic_worker_t<async_worker_t>
{
public:

  using basic_worker_t::basic_worker_t;


private:

  struct impl_t;
  using impl_ptr = std::unique_ptr<impl_t, void(*)(impl_t *)>;

  impl_ptr start ();
  impl_ptr impl_ = start();

  event_ptr make_event (const channel_type &channel) noexcept;
  friend class channel_t<async_worker_t>;
};


__sal_end
}} // namespace sal::logger

/// \}
