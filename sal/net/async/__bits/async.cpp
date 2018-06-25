#include <sal/net/async/__bits/async.hpp>
#include <sal/net/error.hpp>


__sal_begin


namespace net::async::__bits {


#if __sal_os_windows


using ::sal::net::__bits::winsock;


io_block_t::io_block_t (size_t size, io_t::free_list_t &free_list)
  : free_list(free_list)
  , data(new char[size])
  , buffer_id(winsock.RIORegisterBuffer(data.get(), static_cast<DWORD>(size)))
{
  if (buffer_id == RIO_INVALID_BUFFERID)
  {
    std::error_code system_error;
    system_error.assign(::WSAGetLastError(), std::system_category());
    throw_system_error(system_error, "RIORegisterBuffer");
  }

  auto it = data.get(), end = it + size;
  while (it != end)
  {
    free_list.push(new(it) io_t(*this));
    it += sizeof(io_t);
  }
}


io_block_t::~io_block_t () noexcept
{
  winsock.RIODeregisterBuffer(buffer_id);
}


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


io_block_t::io_block_t (size_t size, io_t::free_list_t &free_list)
  : free_list(free_list)
  , data(new char[size])
{
  auto it = data.get(), end = it + size;
  while (it != end)
  {
    free_list.push(new(it) io_t(*this));
    it += sizeof(io_t);
  }
}


io_block_t::~io_block_t () noexcept
{ }


service_t::service_t (std::error_code &) noexcept
{ }


service_t::~service_t () noexcept
{ }


#endif


} // namespace net::async::__bits


__sal_end
