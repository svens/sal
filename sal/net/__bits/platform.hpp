#pragma once

#include <sal/config.hpp>
#if __sal_os_linux || __sal_os_darwin
  #include <netinet/ip.h>
  #include <arpa/inet.h>
#elif __sal_os_windows
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32")
#else
  #error Unsupported platform
#endif


__sal_begin


namespace net { namespace __bits {


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
  auto addr = src;
  return ::inet_ntop(AF_INET6, &addr, dest, size) != nullptr;
#else
  return ::inet_ntop(AF_INET6, &src, dest, size) != nullptr;
#endif
}


inline bool pton (const char *src, in6_addr &dest) noexcept
{
  return ::inet_pton(AF_INET6, src, &dest) == 1;
}


}} // namespace net::__bits


__sal_end
