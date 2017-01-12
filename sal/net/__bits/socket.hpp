#pragma once

#include <sal/config.hpp>
#include <system_error>

#if __sal_os_linux || __sal_os_darwin
  #include <sys/socket.h>
#elif __sal_os_windows
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32")
#else
  #error Unsupported platform
#endif


__sal_begin


namespace net { namespace __bits {


#if __sal_os_windows

  // socket handle
  using native_handle_t = SOCKET;
  constexpr native_handle_t invalid_socket = INVALID_SOCKET;

  // shutdown() direction
  #define SHUT_RD SD_RECEIVE
  #define SHUT_WR SD_SEND
  #define SHUT_RDWR SD_BOTH

  // sockaddr family
  using sa_family_t = ::ADDRESS_FAMILY;

#else

  // socket handle
  using native_handle_t = int;
  constexpr native_handle_t invalid_socket = -1;

  // sockaddr family
  using sa_family_t = ::sa_family_t;

#endif


enum class wait_t { read, write };

native_handle_t open (int domain,
  int type,
  int protocol,
  std::error_code &error
) noexcept;

void close (native_handle_t handle,
  std::error_code &error
) noexcept;

void get_opt (native_handle_t handle,
  int level,
  int name,
  void *data,
  socklen_t *size,
  std::error_code &error
) noexcept;

void set_opt (native_handle_t handle,
  int level,
  int name,
  const void *data,
  socklen_t size,
  std::error_code &error
) noexcept;

bool non_blocking (native_handle_t handle,
  std::error_code &error
) noexcept;

void non_blocking (native_handle_t handle,
  bool mode,
  std::error_code &error
) noexcept;

size_t available (native_handle_t handle,
  std::error_code &error
) noexcept;

void bind (native_handle_t handle,
  const void *address,
  size_t address_size,
  std::error_code &error
) noexcept;

void listen (native_handle_t handle,
  int backlog,
  std::error_code &error
) noexcept;

native_handle_t accept (native_handle_t handle,
  void *address,
  size_t *address_size,
  bool enable_connection_aborted,
  std::error_code &error
) noexcept;

void connect (native_handle_t handle,
  const void *address,
  size_t address_size,
  std::error_code &error
) noexcept;

void shutdown (native_handle_t handle,
  int what,
  std::error_code &error
) noexcept;

bool wait (native_handle_t handle,
  wait_t what,
  int timeout_ms,
  std::error_code &error
) noexcept;

void local_endpoint (native_handle_t handle,
  void *address,
  size_t *address_size,
  std::error_code &error
) noexcept;

void remote_endpoint (native_handle_t handle,
  void *address,
  size_t *address_size,
  std::error_code &error
) noexcept;

size_t recv_from (native_handle_t handle,
  void *data, size_t data_size,
  void *address, size_t *address_size,
  int flags,
  std::error_code &error
) noexcept;

size_t send_to (native_handle_t handle,
  const void *data, size_t data_size,
  const void *address, size_t address_size,
  int flags,
  std::error_code &error
) noexcept;

size_t recv (native_handle_t handle,
  void *data, size_t data_size,
  int flags,
  std::error_code &error
) noexcept;

size_t send (native_handle_t handle,
  const void *data, size_t data_size,
  int flags,
  std::error_code &error
) noexcept;


}} // namespace net::__bits


__sal_end
