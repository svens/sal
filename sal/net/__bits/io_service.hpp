#pragma once

#include <sal/config.hpp>
#include <sal/net/__bits/socket.hpp>


__sal_begin


namespace net { namespace __bits {


#if __sal_os_windows


struct io_buf_t
  : public OVERLAPPED
{
  char *begin, *end;
  uintptr_t user_data;
  size_t request_id;
  DWORD transferred;
  std::error_code error;


  void reset () noexcept
  {
    Internal = InternalHigh = 0;
    Pointer = 0;
    hEvent = 0;
  }


  WSABUF to_buf () const noexcept
  {
    WSABUF buf;
    buf.len = static_cast<ULONG>(end - begin);
    buf.buf = begin;
    return buf;
  }


  bool io_result (int result) noexcept;
};


struct async_receive_t
  : public io_buf_t
{
  bool start (socket_t &socket, message_flags_t flags) noexcept;
};


struct async_receive_from_t
  : public io_buf_t
{
  sockaddr_storage address;
  INT address_size;

  bool start (socket_t &socket, message_flags_t flags) noexcept;
};


struct async_send_to_t
  : public io_buf_t
{
  bool start (socket_t &socket,
    const void *address, size_t address_size,
    message_flags_t flags
  ) noexcept;
};


struct async_send_t
  : public io_buf_t
{
  bool start (socket_t &socket, message_flags_t flags) noexcept;
};


struct async_connect_t
  : public io_buf_t
{
  native_socket_t native_handle;

  bool start (socket_t &socket, const void *address, size_t address_size)
    noexcept;

  void finish (std::error_code &error) noexcept;
};


struct io_service_t
{
  HANDLE iocp = INVALID_HANDLE_VALUE;
  static constexpr size_t max_completions_count = 256;

  io_service_t (std::error_code &error) noexcept;
  ~io_service_t () noexcept;

  void associate (socket_t &socket, std::error_code &error) noexcept;
};


#endif // __sal_os_windows


}} // namespace net::__bits


__sal_end
