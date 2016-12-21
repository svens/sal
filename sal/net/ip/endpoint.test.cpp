#include <sal/net/ip/endpoint.hpp>
#include <sal/net/ip/tcp.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/common.test.hpp>


namespace {


using addr_t = sal::net::ip::address_t;
using addr_v4_t = sal::net::ip::address_v4_t;
using addr_v6_t = sal::net::ip::address_v6_t;


template <typename Protocol>
struct net_ip_endpoint
  : public sal_test::with_type<Protocol>
{
  using endpoint_t = typename Protocol::endpoint_t;
};

using protocol_types = testing::Types<
  sal::net::ip::tcp_t,
  sal::net::ip::udp_t
>;
TYPED_TEST_CASE(net_ip_endpoint, protocol_types);


TYPED_TEST(net_ip_endpoint, ctor)
{
  typename TypeParam::endpoint_t endpoint;
  EXPECT_EQ(TypeParam::v4(), endpoint.protocol());
  EXPECT_EQ(addr_t(), endpoint.address());
  EXPECT_EQ(0U, endpoint.port());
}


TYPED_TEST(net_ip_endpoint, ctor_protocol_v4)
{
  typename TypeParam::endpoint_t endpoint(TypeParam::v4(), 123);
  EXPECT_EQ(addr_v4_t(), endpoint.address());
  EXPECT_EQ(123U, endpoint.port());

  EXPECT_EQ(TypeParam::v4(), endpoint.protocol());
  EXPECT_EQ(TypeParam::v4().type(), endpoint.protocol().type());
  EXPECT_EQ(TypeParam::v4().protocol(), endpoint.protocol().protocol());
}


TYPED_TEST(net_ip_endpoint, ctor_protocol_v6)
{
  typename TypeParam::endpoint_t endpoint(TypeParam::v6(), 123);
  EXPECT_EQ(addr_v6_t(), endpoint.address());
  EXPECT_EQ(123U, endpoint.port());

  EXPECT_EQ(TypeParam::v6(), endpoint.protocol());
  EXPECT_EQ(TypeParam::v6().type(), endpoint.protocol().type());
  EXPECT_EQ(TypeParam::v6().protocol(), endpoint.protocol().protocol());
}


TYPED_TEST(net_ip_endpoint, ctor_address_v4)
{
  typename TypeParam::endpoint_t endpoint(addr_v4_t::loopback(), 123);
  EXPECT_EQ(TypeParam::v4(), endpoint.protocol());
  EXPECT_EQ(addr_v4_t::loopback(), endpoint.address());
  EXPECT_EQ(123U, endpoint.port());
}


TYPED_TEST(net_ip_endpoint, ctor_address_v6)
{
  typename TypeParam::endpoint_t endpoint(addr_v6_t::loopback(), 123);
  EXPECT_EQ(TypeParam::v6(), endpoint.protocol());
  EXPECT_EQ(addr_v6_t::loopback(), endpoint.address());
  EXPECT_EQ(123U, endpoint.port());
}


TYPED_TEST(net_ip_endpoint, address_v4)
{
  typename TypeParam::endpoint_t endpoint(addr_v6_t(), 123);
  EXPECT_EQ(addr_v6_t::any(), endpoint.address());

  endpoint.address(addr_v4_t::loopback());
  EXPECT_EQ(TypeParam::v4(), endpoint.protocol());
  EXPECT_EQ(addr_v4_t::loopback(), endpoint.address());
  EXPECT_EQ(123, endpoint.port());
}


TYPED_TEST(net_ip_endpoint, address_v6)
{
  typename TypeParam::endpoint_t endpoint(addr_v4_t(), 123);
  EXPECT_EQ(addr_v4_t::any(), endpoint.address());

  endpoint.address(addr_v6_t::loopback());
  EXPECT_EQ(TypeParam::v6(), endpoint.protocol());
  EXPECT_EQ(addr_v6_t::loopback(), endpoint.address());
  EXPECT_EQ(123, endpoint.port());
}


TYPED_TEST(net_ip_endpoint, port_v4)
{
  typename TypeParam::endpoint_t endpoint(addr_v4_t(), 123);
  EXPECT_EQ(addr_v4_t::any(), endpoint.address());
  EXPECT_EQ(123, endpoint.port());

  endpoint.port(321);
  EXPECT_EQ(addr_v4_t::any(), endpoint.address());
  EXPECT_EQ(321, endpoint.port());
}


TYPED_TEST(net_ip_endpoint, port_v6)
{
  typename TypeParam::endpoint_t endpoint(addr_v6_t(), 123);
  EXPECT_EQ(addr_v6_t::any(), endpoint.address());
  EXPECT_EQ(123, endpoint.port());

  endpoint.port(321);
  EXPECT_EQ(addr_v6_t::any(), endpoint.address());
  EXPECT_EQ(321, endpoint.port());
}


TYPED_TEST(net_ip_endpoint, const_data_v4)
{
  typename TypeParam::endpoint_t endpoint(addr_v4_t::loopback(), 123);
  EXPECT_EQ(addr_v4_t::loopback(), endpoint.address());
  EXPECT_EQ(123, endpoint.port());

  auto ss = static_cast<const sockaddr_in *>(endpoint.data());
  EXPECT_EQ(AF_INET, ss->sin_family);
  EXPECT_EQ(INADDR_LOOPBACK, ntohl(ss->sin_addr.s_addr));
  EXPECT_EQ(123, ntohs(ss->sin_port));
}


TYPED_TEST(net_ip_endpoint, data_v4)
{
  typename TypeParam::endpoint_t endpoint(addr_v4_t::loopback(), 123);
  EXPECT_EQ(123, endpoint.port());

  auto ss = static_cast<sockaddr_in *>(endpoint.data());
  ss->sin_port = htons(321);
  EXPECT_EQ(321, endpoint.port());
}


TYPED_TEST(net_ip_endpoint, const_data_v6)
{
  typename TypeParam::endpoint_t endpoint(addr_v6_t::loopback(), 123);
  EXPECT_EQ(addr_v6_t::loopback(), endpoint.address());
  EXPECT_EQ(123, endpoint.port());

  auto ss = static_cast<const sockaddr_in6 *>(endpoint.data());
  EXPECT_EQ(AF_INET6, ss->sin6_family);
  EXPECT_TRUE(IN6_IS_ADDR_LOOPBACK(&(ss->sin6_addr)) != 0);
  EXPECT_EQ(123, ntohs(ss->sin6_port));
}


TYPED_TEST(net_ip_endpoint, data_v6)
{
  typename TypeParam::endpoint_t endpoint(addr_v6_t::loopback(), 123);
  EXPECT_EQ(123, endpoint.port());

  auto ss = static_cast<sockaddr_in6 *>(endpoint.data());
  ss->sin6_port = htons(321);
  EXPECT_EQ(321, endpoint.port());
}


TYPED_TEST(net_ip_endpoint, size_v4)
{
  typename TypeParam::endpoint_t endpoint(addr_v4_t::any(), 123);
  EXPECT_EQ(sizeof(sockaddr_in), endpoint.size());
}


TYPED_TEST(net_ip_endpoint, size_v6)
{
  typename TypeParam::endpoint_t endpoint(addr_v6_t::any(), 123);
  EXPECT_EQ(sizeof(sockaddr_in6), endpoint.size());
}


TYPED_TEST(net_ip_endpoint, resize_v4)
{
  typename TypeParam::endpoint_t endpoint(addr_v4_t::any(), 123);
  endpoint.resize(sizeof(sockaddr_in));
  EXPECT_EQ(sizeof(sockaddr_in), endpoint.size());
}


TYPED_TEST(net_ip_endpoint, resize_invalid_v4)
{
  typename TypeParam::endpoint_t endpoint(addr_v4_t::any(), 123);
  EXPECT_THROW(endpoint.resize(sizeof(sockaddr_in6)), std::length_error);
  EXPECT_EQ(sizeof(sockaddr_in), endpoint.size());
}


TYPED_TEST(net_ip_endpoint, resize_v6)
{
  typename TypeParam::endpoint_t endpoint(addr_v6_t::any(), 123);
  endpoint.resize(sizeof(sockaddr_in6));
  EXPECT_EQ(sizeof(sockaddr_in6), endpoint.size());
}


TYPED_TEST(net_ip_endpoint, resize_invalid_v6)
{
  typename TypeParam::endpoint_t endpoint(addr_v6_t::any(), 123);
  EXPECT_THROW(endpoint.resize(sizeof(sockaddr_in)), std::length_error);
  EXPECT_EQ(sizeof(sockaddr_in6), endpoint.size());
}


TYPED_TEST(net_ip_endpoint, capacity_v4)
{
  typename TypeParam::endpoint_t endpoint(addr_v4_t::any(), 123);
  EXPECT_EQ(sizeof(endpoint), endpoint.capacity());
}


TYPED_TEST(net_ip_endpoint, capacity_v6)
{
  typename TypeParam::endpoint_t endpoint(addr_v6_t::any(), 123);
  EXPECT_EQ(sizeof(endpoint), endpoint.capacity());
}


TYPED_TEST(net_ip_endpoint, memory_writer_inserter_v4)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof(data)};
  writer << typename TypeParam::endpoint_t{addr_v4_t::loopback(), 12345};
  EXPECT_STREQ("127.0.0.1:12345", data);
}


TYPED_TEST(net_ip_endpoint, memory_writer_inserter_v6)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof(data)};
  EXPECT_TRUE(
    bool(writer << typename TypeParam::endpoint_t{addr_v6_t::loopback(), 12345})
  );
  EXPECT_STREQ("[::1]:12345", data);
}


TYPED_TEST(net_ip_endpoint, memory_writer_inserter_exact_v4)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof("127.0.0.1:12345") - 1};
  EXPECT_TRUE(
    bool(writer << typename TypeParam::endpoint_t{addr_v4_t::loopback(), 12345})
  );
  EXPECT_STREQ("127.0.0.1:12345", data);
}


TYPED_TEST(net_ip_endpoint, memory_writer_inserter_exact_v6)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof("[::1]:12345") - 1};
  EXPECT_TRUE(
    bool(writer << typename TypeParam::endpoint_t{addr_v6_t::loopback(), 12345})
  );
  EXPECT_STREQ("[::1]:12345", data);
}


TYPED_TEST(net_ip_endpoint, memory_writer_inserter_overflow_v4)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof("127.0.0.1:1234") - 1};
  EXPECT_FALSE(
    bool(writer << typename TypeParam::endpoint_t{addr_v4_t::loopback(), 12345})
  );
}


TYPED_TEST(net_ip_endpoint, memory_writer_inserter_overflow_v6)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof("[::1]:1234") - 1};
  EXPECT_FALSE(
    bool(writer << typename TypeParam::endpoint_t{addr_v6_t::loopback(), 12345})
  );
}


TYPED_TEST(net_ip_endpoint, ostream_inserter_v4)
{
  std::ostringstream oss;
  oss << typename TypeParam::endpoint_t{addr_v4_t::loopback(), 12345};
  EXPECT_EQ("127.0.0.1:12345", oss.str());
}


TYPED_TEST(net_ip_endpoint, ostream_inserter_v6)
{
  std::ostringstream oss;
  oss << typename TypeParam::endpoint_t{addr_v6_t::loopback(), 12345};
  EXPECT_EQ("[::1]:12345", oss.str());
}


} // namespace
