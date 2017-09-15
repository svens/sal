#pragma once

#include <sal/config.hpp>

#if __sal_os_linux || __sal_os_macos
  #include <arpa/inet.h>
  #include <netinet/tcp.h>
  #include <netdb.h>
#elif __sal_os_windows
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32")
#else
  #error Unsupported platform
#endif


__sal_begin


namespace net { namespace ip { namespace __bits {


inline bool inet_ntop (const in_addr &src, char *dest, size_t size) noexcept
{
#if __sal_os_windows
  auto addr = src;
  return ::inet_ntop(AF_INET, &addr, dest, size) != nullptr;
#else
  return ::inet_ntop(AF_INET, &src, dest, size) != nullptr;
#endif
}


inline bool inet_ntop (const in6_addr &src, char *dest, size_t size) noexcept
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


inline bool inet_pton (const char *src, in_addr &dest) noexcept
{
  return ::inet_pton(AF_INET, src, &dest) == 1;
}


inline bool inet_pton (const char *src, in6_addr &dest) noexcept
{
  return ::inet_pton(AF_INET6, src, &dest) == 1;
}


inline uint16_t host_to_network_short (uint16_t v) noexcept
{
  return htons(v);
}


inline uint32_t host_to_network_long (uint32_t v) noexcept
{
  return htonl(v);
}


inline uint16_t network_to_host_short (uint16_t v) noexcept
{
  return ntohs(v);
}


inline uint32_t network_to_host_long (uint32_t v) noexcept
{
  return ntohl(v);
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

#elif __sal_os_macos

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
