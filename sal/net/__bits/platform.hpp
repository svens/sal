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


}}} // namespace net::ip::__bits


__sal_end
