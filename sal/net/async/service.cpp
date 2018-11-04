#include <sal/net/async/service.hpp>


__sal_begin


namespace net::async {


io_t *service_t::alloc_io ()
{
  std::lock_guard lock(io_pool_mutex_);

  auto ctl = free_list_.try_pop();
  if (!ctl)
  {
    auto batch_size = 16 * (1ULL << io_pool_.size());
    auto it = io_pool_.emplace_back(new std::byte[batch_size * sizeof(io_t)]).get();
    io_pool_size_ += batch_size;

    while (batch_size--)
    {
      new(it) io_t(free_list_);
      it += sizeof(io_t);
    }

    ctl = free_list_.try_pop();
  }

  return reinterpret_cast<io_t *>(ctl);
}


} // namespace net::async


__sal_end
