#include <sal/net/ip/address_v4.hpp>
#include <sal/net/fwd.hpp>
#include <sal/common.test.hpp>


namespace {


namespace ip = sal::net::ip;

using net_ip_address_v4 = sal_test::fixture;
using addr_t = sal::net::ip::address_v4_t;


constexpr addr_t::uint_t to_uint (const addr_t::bytes_t &b)
{
  return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}


constexpr addr_t::bytes_t
  null_bytes{{0, 0, 0, 0}},
  some_bytes{{1, 2, 3, 4}},
  loopback_bytes{addr_t::loopback().to_bytes()},
  multicast_bytes{{224, 1, 2, 3}};

constexpr addr_t::uint_t
  null_uint = to_uint(null_bytes),
  some_uint = to_uint(some_bytes);


TEST_F(net_ip_address_v4, ctor)
{
  addr_t a;
  EXPECT_EQ(null_uint, a.to_uint());
  EXPECT_EQ(null_bytes, a.to_bytes());
}


TEST_F(net_ip_address_v4, ctor_bytes)
{
  addr_t a{some_bytes};
  EXPECT_EQ(some_bytes, a.to_bytes());
  EXPECT_EQ(some_uint, a.to_uint());
}


TEST_F(net_ip_address_v4, ctor_uint)
{
  addr_t a{some_uint};
  EXPECT_EQ(some_uint, a.to_uint());
  EXPECT_EQ(some_bytes, a.to_bytes());
}


TEST_F(net_ip_address_v4, ctor_address_v4)
{
  addr_t a{some_bytes}, b{a};
  EXPECT_EQ(some_bytes, b.to_bytes());
  EXPECT_EQ(some_uint, b.to_uint());
}


TEST_F(net_ip_address_v4, operator_assign)
{
  addr_t a{some_bytes}, b;
  b = a;
  EXPECT_EQ(some_bytes, b.to_bytes());
  EXPECT_EQ(some_uint, b.to_uint());
}


TEST_F(net_ip_address_v4, is_unspecified)
{
  addr_t a;
  EXPECT_TRUE(a.is_unspecified());

  addr_t b{some_bytes};
  EXPECT_FALSE(b.is_unspecified());

  EXPECT_TRUE(addr_t::any().is_unspecified());
  EXPECT_FALSE(addr_t::loopback().is_unspecified());
  EXPECT_FALSE(addr_t::broadcast().is_unspecified());
}


TEST_F(net_ip_address_v4, is_loopback)
{
  addr_t a;
  EXPECT_FALSE(a.is_loopback());

  addr_t b{loopback_bytes};
  EXPECT_TRUE(b.is_loopback());

  EXPECT_FALSE(addr_t::any().is_loopback());
  EXPECT_TRUE(addr_t::loopback().is_loopback());
  EXPECT_FALSE(addr_t::broadcast().is_loopback());
}


TEST_F(net_ip_address_v4, is_multicast)
{
  addr_t a;
  EXPECT_FALSE(a.is_multicast());

  addr_t b{multicast_bytes};
  EXPECT_TRUE(b.is_multicast());

  EXPECT_FALSE(addr_t::any().is_multicast());
  EXPECT_FALSE(addr_t::loopback().is_multicast());
  EXPECT_FALSE(addr_t::broadcast().is_multicast());
}


TEST_F(net_ip_address_v4, is_private)
{
  EXPECT_FALSE(addr_t::any().is_private());
  EXPECT_FALSE(addr_t::broadcast().is_private());
  EXPECT_FALSE(addr_t::loopback().is_private());

  //
  // 10.0.0.0 - 10.255.255.255
  //

  EXPECT_FALSE(addr_t{0x0a000000 - 1}.is_private());
  EXPECT_TRUE(addr_t{0x0a000000}.is_private());
  EXPECT_TRUE(addr_t{0x0a000000 + 1}.is_private());

  EXPECT_TRUE(addr_t{0x0affffff - 1}.is_private());
  EXPECT_TRUE(addr_t{0x0affffff}.is_private());
  EXPECT_FALSE(addr_t{0x0affffff + 1}.is_private());

  //
  // 172.16.0.0 - 172.31.255.255
  //

  EXPECT_FALSE(addr_t{0xac100000 - 1}.is_private());
  EXPECT_TRUE(addr_t{0xac100000}.is_private());
  EXPECT_TRUE(addr_t{0xac100000 + 1}.is_private());

  EXPECT_TRUE(addr_t{0xac1fffff - 1}.is_private());
  EXPECT_TRUE(addr_t{0xac1fffff}.is_private());
  EXPECT_FALSE(addr_t{0xac1fffff + 1}.is_private());

  //
  // 192.168.0.0 - 192.168.255.255
  //

  EXPECT_FALSE(addr_t{0xc0a80000 - 1}.is_private());
  EXPECT_TRUE(addr_t{0xc0a80000}.is_private());
  EXPECT_TRUE(addr_t{0xc0a80000 + 1}.is_private());

  EXPECT_TRUE(addr_t{0xc0a8ffff - 1}.is_private());
  EXPECT_TRUE(addr_t{0xc0a8ffff}.is_private());
  EXPECT_FALSE(addr_t{0xc0a8ffff + 1}.is_private());
}


TEST_F(net_ip_address_v4, to_string)
{
  EXPECT_EQ("0.0.0.0", addr_t::any().to_string());
  EXPECT_EQ("127.0.0.1", addr_t::loopback().to_string());
  EXPECT_EQ("255.255.255.255", addr_t::broadcast().to_string());
  EXPECT_EQ("1.2.3.4", addr_t{some_bytes}.to_string());
  EXPECT_EQ("224.1.2.3", addr_t{multicast_bytes}.to_string());
}


TEST_F(net_ip_address_v4, memory_writer_inserter)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof("255.255.255.255")};

  writer << addr_t::any();
  EXPECT_STREQ("0.0.0.0", data);

  writer.first = data;
  writer << addr_t::loopback();
  EXPECT_STREQ("127.0.0.1", data);

  writer.first = data;
  writer << addr_t::broadcast();
  EXPECT_STREQ("255.255.255.255", data);

  writer.first = data;
  writer << addr_t{some_bytes};
  EXPECT_STREQ("1.2.3.4", data);

  writer.first = data;
  writer << addr_t{multicast_bytes};
  EXPECT_STREQ("224.1.2.3", data);
}


TEST_F(net_ip_address_v4, memory_writer_inserter_exact)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof("0.0.0.0")};
  EXPECT_TRUE(bool(writer << addr_t::any()));
  EXPECT_STREQ("0.0.0.0", data);
}


TEST_F(net_ip_address_v4, memory_writer_inserter_overflow)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof("255")};
  EXPECT_FALSE(bool(writer << addr_t::broadcast()));
}


TEST_F(net_ip_address_v4, ostream_inserter)
{
  std::ostringstream oss;

  oss << addr_t::any();
  EXPECT_EQ("0.0.0.0", oss.str());

  oss.str("");
  oss << addr_t::loopback();
  EXPECT_EQ("127.0.0.1", oss.str());

  oss.str("");
  oss << addr_t::broadcast();
  EXPECT_EQ("255.255.255.255", oss.str());

  oss.str("");
  oss << addr_t{some_bytes};
  EXPECT_EQ("1.2.3.4", oss.str());

  oss.str("");
  oss << addr_t{multicast_bytes};
  EXPECT_EQ("224.1.2.3", oss.str());
}


TEST_F(net_ip_address_v4, comparisons)
{
  auto a = addr_t::any();
  auto b = addr_t::broadcast();
  auto c = a;

  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a == c);

  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a != c);

  EXPECT_TRUE(a < b);
  EXPECT_FALSE(a < c);

  EXPECT_FALSE(a > b);
  EXPECT_FALSE(a > c);

  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(a <= c);

  EXPECT_FALSE(a >= b);
  EXPECT_TRUE(a >= c);
}


} // namespace
