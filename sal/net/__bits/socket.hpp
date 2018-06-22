#pragma once

#include <sal/config.hpp>
#include <system_error>

#if __sal_os_macos || __sal_os_linux // {{{1
  #include <sys/socket.h>
#elif __sal_os_windows // {{{1
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32")
#else // {{{1
  #error Unsupported platform
#endif // }}}1


__sal_begin


namespace net { namespace __bits {


const std::error_code &init_lib () noexcept;


#if __sal_os_windows // {{{1

#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH

using sa_family_t = ::ADDRESS_FAMILY;
using message_flags_t = DWORD;

#elif __sal_os_macos || __sal_os_linux // {{{1

using sa_family_t = ::sa_family_t;
using message_flags_t = int;

#endif // }}}1


enum class shutdown_t
{
  receive = SHUT_RD,
  send = SHUT_WR,
  both = SHUT_RDWR,
};


enum class wait_t
{
  read,
  write,
};


struct socket_t
{
#if __sal_os_windows // {{{1
  using handle_t = SOCKET;
  static constexpr handle_t invalid = INVALID_SOCKET;
#elif __sal_os_macos || __sal_os_linux // {{{1
  using handle_t = int;
  static constexpr handle_t invalid = -1;
#endif // }}}1

  handle_t handle = invalid;

  socket_t () = default;

  socket_t (handle_t handle) noexcept
    : handle(handle)
  {}

  socket_t (socket_t &&that) noexcept
    : handle(that.handle)
  {
    that.handle = invalid;
  }

  socket_t &operator= (socket_t &&that) noexcept
  {
    auto tmp = std::move(*this);
    swap(that);
    return *this;
  }

  ~socket_t () noexcept
  {
    if (handle != invalid)
    {
      std::error_code ignored;
      close(ignored);
    }
  }

  socket_t (const socket_t &) = delete;
  socket_t &operator= (const socket_t &) = delete;

  void swap (socket_t &that) noexcept
  {
    using std::swap;
    swap(handle, that.handle);
  }

  void open (int domain,
    int type,
    int protocol,
    std::error_code &error
  ) noexcept;

  void close (std::error_code &error) noexcept;

  void bind (const void *address, size_t address_size, std::error_code &error)
    noexcept;

  void listen (int backlog, std::error_code &error) noexcept;

  handle_t accept (void *address, size_t *address_size,
    bool enable_connection_aborted,
    std::error_code &error
  ) noexcept;

  void connect (const void *address, size_t address_size,
    std::error_code &error
  ) noexcept;

  bool wait (wait_t what,
    int timeout_ms,
    std::error_code &error
  ) noexcept;

  void shutdown (shutdown_t what, std::error_code &error) noexcept;

  void remote_endpoint (void *address, size_t *address_size,
    std::error_code &error
  ) const noexcept;

  void local_endpoint (void *address, size_t *address_size,
    std::error_code &error
  ) const noexcept;

  void get_opt (int level, int name,
    void *data, size_t *size,
    std::error_code &error
  ) const noexcept;

  void set_opt (int level, int name,
    const void *data, size_t size,
    std::error_code &error
  ) noexcept;

  bool non_blocking (std::error_code &error) const noexcept;
  void non_blocking (bool mode, std::error_code &error) noexcept;

  size_t available (std::error_code &error) const noexcept;

  size_t receive (void *data, size_t data_size,
    message_flags_t flags,
    std::error_code &error
  ) noexcept;

  size_t receive_from (void *data, size_t data_size,
    void *address, size_t *address_size,
    message_flags_t flags,
    std::error_code &error
  ) noexcept;

  size_t send (const void *data, size_t data_size,
    message_flags_t flags,
    std::error_code &error
  ) noexcept;

  size_t send_to (const void *data, size_t data_size,
    const void *address, size_t address_size,
    message_flags_t flags,
    std::error_code &error
  ) noexcept;
};


}} // namespace net::__bits


__sal_end
