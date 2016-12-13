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


class lib_t
{
public:

  static void startup () noexcept;


private:

  static lib_t instance;

  lib_t () noexcept
  {
    startup();
  }

  ~lib_t () noexcept;
};


inline bool ntop (const in_addr &addr, char *dest, size_t size) noexcept
{
  return ::inet_ntop(AF_INET, &addr, dest, size) != nullptr;
}


}} // namespace net::__bits


__sal_end
