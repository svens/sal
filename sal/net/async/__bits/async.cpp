#include <sal/__bits/platform_sdk.hpp>
#include <sal/net/async/__bits/async.hpp>


__sal_begin


namespace net::async::__bits {


#if __sal_os_windows //{{{1


namespace {

using net::__bits::winsock;
constexpr DWORD acceptex_address_size = sizeof(sockaddr_storage) + 16;


inline WSABUF make_buf (io_t *io) noexcept
{
  WSABUF result;
  result.buf = reinterpret_cast<CHAR *>(io->begin);
  result.len = static_cast<ULONG>(io->end - io->begin);
  return result;
}


inline void io_result_handle (io_t *io, int result) noexcept
{
  if (result == 0)
  {
    io->status.clear();
    io->current_owner->service->enqueue(io);
    return;
  }

  auto e = ::WSAGetLastError();
  if (e != WSA_IO_PENDING)
  {
    io->status.assign(e, std::system_category());
    io->current_owner->service->enqueue(io);
  }
}


inline int complete_connection (io_t &io) noexcept
{
  // see worker_t::result_at()
  if (io.lib_context == SO_UPDATE_ACCEPT_CONTEXT)
  {
    return ::setsockopt(
      *io.pending.accept.socket_handle,
      SOL_SOCKET,
      SO_UPDATE_ACCEPT_CONTEXT,
      reinterpret_cast<char *>(&io.current_owner->handle),
      sizeof(io.current_owner->handle)
    );
  }
  else if (io.lib_context == SO_UPDATE_CONNECT_CONTEXT)
  {
    return ::setsockopt(
      io.current_owner->handle,
      SOL_SOCKET,
      SO_UPDATE_CONNECT_CONTEXT,
      nullptr,
      0
    );
  }
  return !SOCKET_ERROR;
}


} // namespace


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
  auto status = static_cast<NTSTATUS>(io.overlapped.Internal);

  if (NT_SUCCESS(status))
  {
    // ugly hack: AcceptEx/ConnectEx must be accompanied with setsockopt()
    // If io.transferred points to &io.lib_context, then complete connection
    // with separate call

    if (io.transferred != &io.lib_context
      || complete_connection(io) != SOCKET_ERROR)
    {
      io.status.clear();
    }
    else
    {
      io.status.assign(::WSAGetLastError(), std::system_category());
    }
  }
  else
  {
    switch (status)
    {
      case STATUS_BUFFER_OVERFLOW:
        io.status = std::make_error_code(std::errc::message_size);
        break;

      case STATUS_INVALID_ADDRESS_COMPONENT:
        io.status = std::make_error_code(std::errc::address_not_available);
        break;

      case STATUS_CONNECTION_REFUSED:
        io.status = std::make_error_code(std::errc::connection_refused);
        break;

      default:
        io.status.assign(
          ::RtlNtStatusToDosError(status),
          std::system_category()
        );
        break;
    }
  }

  *io.transferred = result.dwNumberOfBytesTransferred;
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


void handler_t::start_receive_from (io_t *io,
  void *remote_endpoint,
  size_t remote_endpoint_capacity,
  size_t *transferred,
  message_flags_t *flags) noexcept
{
  io->current_owner = this;
  io->transferred = transferred;

  io->pending.recv_from.remote_endpoint_capacity =
    static_cast<INT>(remote_endpoint_capacity);
  io->pending.recv_from.flags = flags;

  auto buf = make_buf(io);
  auto result = ::WSARecvFrom(
    handle,
    &buf,
    1,
    &io->pending.recv_from.transferred,
    io->pending.recv_from.flags,
    static_cast<sockaddr *>(remote_endpoint),
    &io->pending.recv_from.remote_endpoint_capacity,
    &io->overlapped,
    nullptr
  );

  if (result == 0)
  {
    *io->transferred = io->pending.recv_from.transferred;
  }

  io_result_handle(io, result);
}


void handler_t::start_receive (io_t *io,
  size_t *transferred,
  message_flags_t *flags) noexcept
{
  io->current_owner = this;
  io->transferred = transferred;

  io->pending.receive.flags = flags;

  auto buf = make_buf(io);
  auto result = ::WSARecv(
    handle,
    &buf,
    1,
    &io->pending.receive.transferred,
    io->pending.receive.flags,
    &io->overlapped,
    nullptr
  );

  if (result == 0)
  {
    *io->transferred = io->pending.receive.transferred;
  }

  io_result_handle(io, result);
}


void handler_t::start_send_to (io_t *io,
  const void *remote_endpoint,
  size_t remote_endpoint_size,
  size_t *transferred,
  message_flags_t flags) noexcept
{
  io->current_owner = this;
  io->transferred = transferred;

  auto buf = make_buf(io);
  auto result = ::WSASendTo(
    handle,
    &buf,
    1,
    &io->pending.send_to.transferred,
    flags,
    static_cast<const sockaddr *>(remote_endpoint),
    static_cast<int>(remote_endpoint_size),
    &io->overlapped,
    nullptr
  );

  if (result == 0)
  {
    *io->transferred = io->pending.send_to.transferred;
  }

  io_result_handle(io, result);
}


void handler_t::start_send (io_t *io,
  size_t *transferred,
  message_flags_t flags) noexcept
{
  io->current_owner = this;
  io->transferred = transferred;

  auto buf = make_buf(io);
  auto result = ::WSASend(
    handle,
    &buf,
    1,
    &io->pending.send.transferred,
    flags,
    &io->overlapped,
    nullptr
  );

  if (result == 0)
  {
    *io->transferred = io->pending.send.transferred;
  }

  io_result_handle(io, result);
}


void handler_t::start_accept (io_t *io,
  int family,
  socket_t::handle_t *socket_handle) noexcept
{
  io->current_owner = this;
  io->pending.accept.socket_handle = socket_handle;

  // see worker_t::result_at
  io->lib_context = SO_UPDATE_ACCEPT_CONTEXT;
  io->transferred = &io->lib_context;

  socket_t new_socket;
  new_socket.open(family, SOCK_STREAM, IPPROTO_TCP, io->status);
  *io->pending.accept.socket_handle = new_socket.handle;

  if (io->status)
  {
    io->current_owner->service->enqueue(io);
    return;
  }

  new_socket.handle = socket_t::invalid;

  auto success = winsock.AcceptEx(
    handle,
    *io->pending.accept.socket_handle,
    io->data,
    0,
    0,
    acceptex_address_size,
    nullptr,
    &io->overlapped
  );

  io_result_handle(io,
    success == TRUE ? complete_connection(*io) : SOCKET_ERROR
  );
}


void handler_t::start_connect (io_t *io,
  const void *remote_endpoint,
  size_t remote_endpoint_size) noexcept
{
  io->current_owner = this;

  // see worker_t::result_at
  io->lib_context = SO_UPDATE_CONNECT_CONTEXT;
  io->transferred = &io->lib_context;

  auto success = winsock.ConnectEx(
    handle,
    static_cast<const sockaddr *>(remote_endpoint),
    static_cast<int>(remote_endpoint_size),
    nullptr,
    0,
    nullptr,
    &io->overlapped
  );

  io_result_handle(io,
    success == TRUE ? complete_connection(*io) : SOCKET_ERROR
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


void handler_t::start_receive (io_t *io,
  size_t *transferred,
  message_flags_t *flags) noexcept
{
  (void)io;
  (void)transferred;
  (void)flags;
}


void handler_t::start_send_to (io_t *io,
  const void *remote_endpoint,
  size_t remote_endpoint_size,
  size_t *transferred,
  message_flags_t flags) noexcept
{
  (void)io;
  (void)remote_endpoint;
  (void)remote_endpoint_size;
  (void)transferred;
  (void)flags;
}


void handler_t::start_send (io_t *io,
  size_t *transferred,
  message_flags_t flags) noexcept
{
  (void)io;
  (void)transferred;
  (void)flags;
}


void handler_t::start_accept (io_t *io,
  int family,
  socket_t::handle_t *socket_handle) noexcept
{
  (void)io;
  (void)family;
  (void)socket_handle;
}


void handler_t::start_connect (io_t *io,
  const void *remote_endpoint,
  size_t remote_endpoint_size) noexcept
{
  (void)io;
  (void)remote_endpoint;
  (void)remote_endpoint_size;
}


#endif //}}}1


} // namespace net::async::__bits


__sal_end
