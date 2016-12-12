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


namespace net { namespace ip { namespace __bits {


class lib_t
{
public:

  static lib_t instance;

private:

  lib_t ();
};


inline bool ntop (const void *addr, char *dest, size_t size) noexcept
{
  return ::inet_ntop(AF_INET, addr, dest, size) != nullptr;
}


}}} // namespace net::ip::__bits


__sal_end
