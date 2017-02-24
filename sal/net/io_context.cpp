#include <sal/net/io_context.hpp>


#if __sal_os_windows
__sal_begin


namespace net {


void io_context_t::extend_pool ()
{
  pool_.emplace_back();
  auto &slot = pool_.back();
  char *it = slot.data(), * const e = slot.data() + slot.size();
  for (/**/;  it != e;  it += sizeof(io_buf_t))
  {
    free_.push(new(it) io_buf_t(this));
  }
}


} // namespace net


__sal_end
#endif // __sal_os_windows
