#include <sal/net/io_context.hpp>

#if __sal_os_windows
  #include <winternl.h>
  #if !defined(STATUS_BUFFER_OVERFLOW)
    static const int STATUS_BUFFER_OVERFLOW = 0x80000005;
  #endif
  #pragma comment(lib, "ntdll")
#endif


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
#if __sal_os_windows

  OVERLAPPED_ENTRY entries[64];
  ULONG completed_count;

  auto succeeded = ::GetQueuedCompletionStatusEx(poller_,
    entries, static_cast<ULONG>(max_wait_completed_),
    &completed_count,
    static_cast<DWORD>(period.count()),
    false
  );

  if (!succeeded)
  {
    auto e = ::GetLastError();
    if (e != WAIT_TIMEOUT)
    {
      error.assign(e, std::system_category());
    }
    return false;
  }

  for (auto i = 0U;  i < completed_count;  ++i)
  {
    auto &entry = entries[i];
    auto io_buf = static_cast<io_buf_t *>(entry.lpOverlapped);

    if (auto status = static_cast<NTSTATUS>(io_buf->Internal))
    {
      auto &r = *reinterpret_cast<__bits::async_t *>(io_buf->request_data_);
      if (status == STATUS_BUFFER_OVERFLOW)
      {
        r.error_.assign(WSAEMSGSIZE, std::system_category());
      }
      else
      {
        r.error_.assign(::RtlNtStatusToDosError(status),
          std::system_category()
        );
      }
    }

    io_buf->resize(entry.dwNumberOfBytesTransferred);
    io_buf->this_context_ = this;
    completed_.push(io_buf);
  }

  return completed_count > 0;

#else

  (void)period;
  (void)error;
  return false;

#endif
}


} // namespace net


__sal_end
