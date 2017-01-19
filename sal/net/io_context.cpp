#include <sal/net/io_context.hpp>


__sal_begin


namespace net {


bool io_context_t::extend_pool () noexcept
{
  try
  {
    pool_.emplace_back();
    auto &slot = pool_.back();
    char *it = slot.data(), * const e = slot.data() + slot.size();
    for (/**/;  it != e;  it += sizeof(io_buf_t))
    {
      free_.push(new(it) io_buf_t(this));
    }
    return true;
  }
  catch (const std::bad_alloc &)
  {
    return false;
  }
}


bool io_context_t::wait_for_more (const std::chrono::milliseconds &period,
  std::error_code &error) noexcept
{
  __bits::poller_record_t entries[64];
  auto completed_count = poller_.wait(period,
    entries, wait_max_completed_,
    error
  );

  for (auto i = 0U;  i < completed_count;  ++i)
  {
    auto &entry = entries[i];
    io_buf_t *io_buf;

#if __sal_os_windows

    io_buf = static_cast<io_buf_t *>(entry.lpOverlapped);
    io_buf->socket_data_ = entry.lpCompletionKey;
    io_buf->resize(entry.dwNumberOfBytesTransferred);

#else

    (void)entry;
    io_buf = nullptr;

#endif

    io_buf->this_context_ = this;
    completed_.push(io_buf);
  }

  return completed_count > 0;
}


} // namespace net


__sal_end
