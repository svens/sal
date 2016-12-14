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


inline bool ntop (const in_addr &addr, char *dest, size_t size) noexcept
{
#if __sal_os_windows
  auto a = addr;
  return ::inet_ntop(AF_INET, &a, dest, size) != nullptr;
#else
  return ::inet_ntop(AF_INET, &addr, dest, size) != nullptr;
#endif
}


}} // namespace net::__bits


__sal_end
