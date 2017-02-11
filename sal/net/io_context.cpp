#if defined(_WIN32) || defined(_WIN64)
  #define WIN32_NO_STATUS
  #include <windows.h>
  #undef WIN32_NO_STATUS
  #include <winternl.h>
  #include <ntstatus.h>
  #pragma comment(lib, "ntdll")
#endif

#include <sal/net/io_context.hpp>


#if __sal_os_windows
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
  OVERLAPPED_ENTRY entries[max_completion_count];
  ULONG completed_count;

  auto succeeded = ::GetQueuedCompletionStatusEx(io_service_.iocp,
    entries, static_cast<ULONG>(completion_count_),
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

    auto status = static_cast<NTSTATUS>(io_buf->Internal);
    if (!NT_SUCCESS(status))
    {
      if (status == STATUS_BUFFER_OVERFLOW)
      {
        io_buf->error.assign(WSAEMSGSIZE, std::system_category());
      }
      else
      {
        io_buf->error.assign(::RtlNtStatusToDosError(status),
          std::system_category()
        );
      }
    }

    io_buf->transferred = entry.dwNumberOfBytesTransferred;
    io_buf->this_context_ = this;
    completed_.push(io_buf);
  }

  return completed_count > 0;
}


} // namespace net


__sal_end
#endif // __sal_os_windows
