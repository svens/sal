#include <sal/logger/__bits/logger.hpp>
#include <sal/logger/sink.hpp>


namespace sal { namespace logger {
__sal_begin


namespace __bits {


level_t logger_base_t::default_threshold () noexcept
{
  return level_t::DEBUG;
}


sink_ptr logger_base_t::default_sink () noexcept
{
  static auto sink_ = std::make_shared<sink_t>();
  return sink_;
}


} // namespace __bits


__sal_end
}} // namespace sal::logger
