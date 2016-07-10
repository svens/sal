#include <sal/logger/__bits/logger.hpp>
#include <sal/logger/event.hpp>
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
  struct default_sink_t final
    : public sink_base_t
  {
    void event_write (event_t &event) final override
    {
      if (event.message.good())
      {
        printf("%s\n", event.message.get());
      }
      else
      {
        // message is truncated, append marker
        // (message itself is always NUL-terminated)
        printf("%s<...>\n", event.message.get());
      }
    }
  };

  static auto sink_ = std::make_shared<default_sink_t>();
  return sink_;
}


} // namespace __bits


__sal_end
}} // namespace sal::logger
