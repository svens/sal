#include <sal/net/async/__bits/async.hpp>
#include <sal/net/error.hpp>


__sal_begin


namespace net::async::__bits {


#if __sal_os_windows


using ::sal::net::__bits::winsock;


namespace {


void register_block (context_t *owner,
  io_t::free_list &free_list,
  char *begin,
  char *end)
{
  auto buf_id = winsock.RIORegisterBuffer(begin, static_cast<ULONG>(end - begin));
  if (buf_id == RIO_INVALID_BUFFERID)
  {
    std::error_code system_error;
    system_error.assign(::WSAGetLastError(), std::system_category());
    throw_system_error(system_error, "RIORegisterBuffer");
  }

  for (auto it = begin;  it != end;  it += sizeof(io_t))
  {
    auto io = new(it) io_t(owner);
    io->buf.BufferId = buf_id;
    io->buf.Offset = static_cast<ULONG>(it - begin);
    io->buf.Length = sizeof(io_t);
    free_list.push(io);
  }
}


} // namespace


service_t::service_t (std::error_code &error) noexcept
  : iocp(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0))
{
  if (iocp == INVALID_HANDLE_VALUE)
  {
    error.assign(::GetLastError(), std::system_category());
  }
}


service_t::~service_t () noexcept
{
  if (iocp != INVALID_HANDLE_VALUE)
  {
    (void)::CloseHandle(iocp);
  }
}


#elif __sal_os_linux || __sal_os_macos


namespace {


void register_block (context_t *owner,
  io_t::free_list &free_list,
  char *begin,
  char *end)
{
  while (begin != end)
  {
    free_list.push(new(begin) io_t(owner));
    begin += sizeof(io_t);
  }
}


} // namespace


service_t::service_t (std::error_code &) noexcept
{ }


service_t::~service_t () noexcept
{ }


#endif


io_t *context_t::make_io (uintptr_t type, void *user_data)
{
  auto io = free.try_pop();
  if (!io)
  {
    // extend pool (each next block has 2x size of previous one)
    size_t block_size = 256;
    block_size *= (1ULL << pool.size());
    block_size *= sizeof(io_t);

    auto block = pool.emplace_back(new char[block_size]).get();
    register_block(this, free, block, block + block_size);
    service->io_pool_size += block_size;

    io = free.try_pop();
  }
  io->this_context = this;
  io->user_data_type = type;
  io->user_data = user_data;
  return static_cast<io_t *>(io);
}


} // namespace net::async::__bits


__sal_end
