#include <sal/__bits/platform_sdk.hpp>
#include <sal/net/async/__bits/async.hpp>


__sal_begin


namespace net::async::__bits {


#if __sal_os_windows //{{{1


service_t::service_t ()
  : iocp(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0))
{
  if (iocp == INVALID_HANDLE_VALUE)
  {
    std::error_code error;
    error.assign(::GetLastError(), std::system_category());
    throw_system_error(error, "CreateIoCompletionPort");
  }
}


service_t::~service_t () noexcept
{
  if (iocp != INVALID_HANDLE_VALUE)
  {
    (void)::CloseHandle(iocp);
  }
}


bool worker_t::wait_for_more (const std::chrono::milliseconds &timeout,
  std::error_code &error) noexcept
{
  ULONG event_count;
  auto succeeded = ::GetQueuedCompletionStatusEx(
    service->iocp,
    completed.data(),
    static_cast<DWORD>(max_results_per_poll),
    &event_count,
    static_cast<DWORD>(timeout.count()),
    false
  );

  if (succeeded)
  {
    first_completed = completed.begin();
    last_completed = first_completed + event_count;
    return true;
  }

  auto e = ::GetLastError();
  if (e == WAIT_TIMEOUT)
  {
    error.clear();
  }
  else
  {
    error.assign(e, std::system_category());
  }

  first_completed = last_completed = completed.begin();
  return false;
}


io_t *worker_t::result_at (typename completed_array_t::const_iterator it)
  noexcept
{
  auto &result = *it;
  auto &io = *reinterpret_cast<io_t *>(result.lpOverlapped);
  *io.transferred = result.dwNumberOfBytesTransferred;
  auto status = static_cast<NTSTATUS>(io.overlapped.Internal);

  if (NT_SUCCESS(status))
  {
    io.status.clear();
  }
  else
  {
    if (status == STATUS_BUFFER_OVERFLOW)
    {
      io.status.assign(WSAEMSGSIZE, std::system_category());
    }
    else
    {
      io.status.assign(::RtlNtStatusToDosError(status), std::system_category());
    }
  }

  return &io;
}


handler_t::handler_t (service_ptr service,
    socket_t &socket,
    std::error_code &error) noexcept
  : service(service)
  , handle(socket.handle)
{
  auto result = ::CreateIoCompletionPort(
    reinterpret_cast<HANDLE>(handle),
    service->iocp,
    0,
    0
  );
  if (!result)
  {
    error.assign(::GetLastError(), std::system_category());
  }
}


namespace {


inline auto make_buf (io_t *io) noexcept
{
  WSABUF result;
  result.buf = reinterpret_cast<CHAR *>(io->begin);
  result.len = static_cast<ULONG>(io->end - io->begin);
  return result;
}


void io_result_check (io_t *io, int result) noexcept
{
  // caller still owns io

  if (result == 0)
  {
    // completed immediately, move ownership to library
    io->status.clear();
    *io->transferred = io->pending.transferred;
    io->current_owner->service->enqueue(io);
    return;
  }

  auto e = ::WSAGetLastError();
  if (e == WSA_IO_PENDING)
  {
    // pending, OS owns io
    return;
  }

  // failed, move ownership to library
  else
  {
    io->status.assign(e, std::system_category());
  }

  io->current_owner->service->enqueue(io);
}


} // namespace


void handler_t::start_receive_from (io_t *io,
  void *remote_endpoint,
  size_t remote_endpoint_capacity,
  size_t *transferred,
  message_flags_t *flags) noexcept
{
  io->current_owner = this;
  io->pending.recv_from.remote_endpoint_capacity =
    static_cast<INT>(remote_endpoint_capacity);
  io->transferred = transferred;
  io->flags = flags;

  auto buf = make_buf(io);
  io_result_check(io,
    ::WSARecvFrom(
      handle,
      &buf,
      1,
      &io->pending.transferred,
      io->flags,
      static_cast<sockaddr *>(remote_endpoint),
      &io->pending.recv_from.remote_endpoint_capacity,
      &io->overlapped,
      nullptr
    )
  );
}


#elif __sal_os_linux || __sal_os_macos //{{{1


service_t::service_t ()
{ }


service_t::~service_t () noexcept
{ }


bool worker_t::wait_for_more (const std::chrono::milliseconds &timeout,
  std::error_code &error) noexcept
{
  (void)timeout;
  (void)error;
  return false;
}


io_t *worker_t::result_at (typename completed_array_t::const_iterator it)
  noexcept
{
  (void)it;
  return nullptr;
}


handler_t::handler_t (service_ptr service,
    socket_t &socket,
    std::error_code &error) noexcept
  : service(service)
  , handle(socket.handle)
{
  error.clear();
}


void handler_t::start_receive_from (io_t *io,
  void *remote_endpoint,
  size_t remote_endpoint_capacity,
  size_t *transferred,
  message_flags_t *flags) noexcept
{
  (void)io;
  (void)remote_endpoint;
  (void)remote_endpoint_capacity;
  (void)transferred;
  (void)flags;
}


#endif //}}}1


} // namespace net::async::__bits


__sal_end
