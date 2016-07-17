#include <sal/logger/__bits/channel.hpp>
#include <sal/logger/sink.hpp>


namespace sal { namespace logger {
__sal_begin


namespace __bits {


sink_ptr channel_base_t::default_sink () noexcept
{
  static auto sink_ = std::make_shared<sink_t>();
  return sink_;
}


} // namespace __bits


__sal_end
}} // namespace sal::logger
