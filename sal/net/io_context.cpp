#include <sal/net/io_context.hpp>


__sal_begin


namespace net {


bool io_context_t::wait (size_t timeout_milliseconds, std::error_code &error)
  noexcept
{
  (void)timeout_milliseconds;
  (void)error;
  return false;
}


io_buf_t *io_context_t::next () noexcept
{
  return nullptr;
}


} // namespace net


__sal_end
