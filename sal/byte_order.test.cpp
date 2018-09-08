#include <sal/byte_order.hpp>
#include <sal/common.test.hpp>
#include <array>
#include <limits>

#if __sal_os_linux || __sal_os_macos
  #include <arpa/inet.h>
#elif __sal_os_windows
  #include <winsock2.h>
#else
  #error Unsupported platform
#endif


namespace {


#if defined(__sal_os_linux)
  #if __sal_byte_order == __sal_little_endian
    #define htonll(x) __builtin_bswap64(x)
    #define ntohll(x) __builtin_bswap64(x)
  #else
    #define htonll(x) (x)
    #define ntohll(x) (x)
  #endif
#endif


inline auto hton (uint16_t v) noexcept
{
  return htons(v);
}


inline auto hton (uint32_t v) noexcept
{
  return htonl(v);
}


inline auto hton (uint64_t v) noexcept
{
  return htonll(v);
}


inline auto ntoh (uint16_t v) noexcept
{
  return ntohs(v);
}


inline auto ntoh (uint32_t v) noexcept
{
  return ntohl(v);
}


inline auto ntoh (uint64_t v) noexcept
{
  return ntohll(v);
}


template <typename T>
constexpr auto test_data () noexcept
{
  using limits = std::numeric_limits<T>;

  return std::array<T, 4>
  {{
    (limits::min)(),
    (limits::lowest)(),
    (limits::max)(),
    static_cast<T>((limits::lowest)() + (limits::max)() / 2),
  }};
}


template <typename T>
using byte_order = sal_test::with_type<T>;

using int_types = testing::Types<
  uint16_t,
  uint32_t,
  uint64_t
>;

TYPED_TEST_CASE(byte_order, int_types, );


TYPED_TEST(byte_order, native_to_network)
{
  for (auto value: test_data<TypeParam>())
  {
    EXPECT_EQ(hton(value), sal::native_to_network_byte_order(value));
  }
}


TYPED_TEST(byte_order, network_to_native)
{
  for (auto value: test_data<TypeParam>())
  {
    EXPECT_EQ(ntoh(value), sal::network_to_native_byte_order(value));
  }
}


} // namespace
