#pragma once

#include <sal/config.hpp>
#include <sal/error.hpp>
#if __sal_os_linux || __sal_os_darwin
  #include <netinet/ip.h>
  #include <arpa/inet.h>
  #include <netdb.h>
#elif __sal_os_windows
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32")
#else
  #error Unsupported platform
#endif


__sal_begin


namespace net {

namespace __bits {


struct error_guard
{
  std::error_code error{};
  const char * const msg;

  error_guard () = delete;
  error_guard (const error_guard &) = delete;
  error_guard &operator= (const error_guard &) = delete;

  error_guard (const char *msg) noexcept
    : msg(msg)
  {}

  ~error_guard () noexcept(false)
  {
    if (error)
    {
      throw_system_error(error, msg);
    }
  }

  operator std::error_code& ()
  {
    return error;
  }
};


#if __sal_os_windows
  using native_handle_t = SOCKET;
  const native_handle_t invalid_socket = INVALID_SOCKET;
  #define SHUT_RD SD_RECEIVE
  #define SHUT_WR SD_SEND
  #define SHUT_RDWR SD_BOTH
#else
  using native_handle_t = int;
  const native_handle_t invalid_socket = -1;
#endif

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

void local_endpoint (native_handle_t handle,
  void *address,
  size_t *address_size,
  std::error_code &error
) noexcept;

} // namespace __bits


namespace ip { namespace __bits {


#if __sal_os_windows
  using sa_family_t = ::ADDRESS_FAMILY;
#else
  using sa_family_t = ::sa_family_t;
#endif


inline bool ntop (const in_addr &src, char *dest, size_t size) noexcept
{
#if __sal_os_windows
  auto addr = src;
  return ::inet_ntop(AF_INET, &addr, dest, size) != nullptr;
#else
  return ::inet_ntop(AF_INET, &src, dest, size) != nullptr;
#endif
}


inline bool pton (const char *src, in_addr &dest) noexcept
{
  return ::inet_pton(AF_INET, src, &dest) == 1;
}


inline bool ntop (const in6_addr &src, char *dest, size_t size) noexcept
{
#if __sal_os_windows
  // depending on SDK version, it's API parameter is const or not
  // make it "void *" regardless
  auto addr = &const_cast<in6_addr &>(src);
  return ::inet_ntop(AF_INET6, addr, dest, size) != nullptr;
#else
  return ::inet_ntop(AF_INET6, &src, dest, size) != nullptr;
#endif
}


inline bool pton (const char *src, in6_addr &dest) noexcept
{
  return ::inet_pton(AF_INET6, src, &dest) == 1;
}


#if defined(_MSC_VER)
  #define CONSTEXPR inline
#else
  #define CONSTEXPR constexpr
#endif


CONSTEXPR uint64_t fnv_1a (const uint8_t *first, const uint8_t *last) noexcept
{
  auto h = 0xcbf29ce484222325ULL;
  while (first != last)
  {
    h ^= *first++;
    h += (h << 1) + (h << 4) + (h << 5) + (h << 7) + (h << 8) + (h << 40);
  }
  return h;
}


CONSTEXPR uint64_t combine (uint64_t h, uint64_t l) noexcept
{
  constexpr uint64_t mul = 0x9ddfea08eb382d69ULL;
  uint64_t a = (l ^ h) * mul;
  a ^= (a >> 47);
  uint64_t b = (h ^ a) * mul;
  b ^= (b >> 47);
  return b * mul;
}


inline int to_gai_error (int sys_error,
  const char *host_name,
  const char *service_name) noexcept
{
  (void)host_name;
  (void)service_name;

#if __sal_os_windows

  switch (sys_error)
  {
    case WSATRY_AGAIN:
      return EAI_AGAIN;
    case WSAEINVAL:
      return EAI_BADFLAGS;
    case WSANO_RECOVERY:
      return EAI_FAIL;
    case WSAEAFNOSUPPORT:
      return EAI_FAMILY;
    case WSA_NOT_ENOUGH_MEMORY:
      return EAI_MEMORY;
    case WSAHOST_NOT_FOUND:
      return EAI_NONAME;
    case WSATYPE_NOT_FOUND:
      return EAI_SERVICE;
    case WSAESOCKTNOSUPPORT:
      return EAI_SOCKTYPE;
  }

#elif __sal_os_darwin

  if (sys_error == EAI_NONAME && (!host_name || !*host_name))
  {
    // Darwin returns EAI_NONAME if either host or service is not found
    // other platforms return EAI_SERVICE if service is not found
    // align Darwin to other platforms
    sys_error = EAI_SERVICE;
  }

#endif

  return sys_error;
}


}}} // namespace net::ip::__bits


__sal_end
