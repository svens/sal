#include <sal/net/ip/address_v4.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_ip_address_v4
  : public sal_test::fixture
{
  using addr_t = sal::net::ip::address_v4_t;

  addr_t::bytes_t null =
  {
    { 0, 0, 0, 0 }
  };

  addr_t::bytes_t bytes =
  {
    { 1, 2, 3, 4 }
  };

  addr_t::bytes_t multicast =
  {
    { 224, 1, 2, 3 }
  };

  static constexpr addr_t::uint_t to_uint (const addr_t::bytes_t &b) noexcept
  {
    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
  }
};


TEST_F(net_ip_address_v4, ctor)
{
  addr_t a;
  EXPECT_EQ(to_uint(null), a.to_uint());
  EXPECT_EQ(null, a.to_bytes());
}


TEST_F(net_ip_address_v4, ctor_bytes)
{
  addr_t a{bytes};
  EXPECT_EQ(bytes, a.to_bytes());
  EXPECT_EQ(to_uint(bytes), a.to_uint());
}


TEST_F(net_ip_address_v4, ctor_uint)
{
  addr_t a{to_uint(bytes)};
  EXPECT_EQ(to_uint(bytes), a.to_uint());
  EXPECT_EQ(bytes, a.to_bytes());
}


TEST_F(net_ip_address_v4, ctor_address_v4)
{
  addr_t a{bytes}, b{a};
  EXPECT_EQ(bytes, b.to_bytes());
  EXPECT_EQ(to_uint(bytes), b.to_uint());
}


TEST_F(net_ip_address_v4, operator_assign)
{
  addr_t a{bytes}, b;
  b = a;
  EXPECT_EQ(bytes, b.to_bytes());
  EXPECT_EQ(to_uint(bytes), b.to_uint());
}


TEST_F(net_ip_address_v4, is_unspecified)
{
  addr_t a;
  EXPECT_TRUE(a.is_unspecified());

  addr_t b{bytes};
  EXPECT_FALSE(b.is_unspecified());

  EXPECT_TRUE(addr_t::any().is_unspecified());
  EXPECT_FALSE(addr_t::loopback().is_unspecified());
  EXPECT_FALSE(addr_t::broadcast().is_unspecified());
}


TEST_F(net_ip_address_v4, is_loopback)
{
  addr_t a;
  EXPECT_FALSE(a.is_loopback());

  addr_t b{bytes};
  EXPECT_FALSE(b.is_loopback());

  EXPECT_FALSE(addr_t::any().is_loopback());
  EXPECT_TRUE(addr_t::loopback().is_loopback());
  EXPECT_FALSE(addr_t::broadcast().is_loopback());
}


TEST_F(net_ip_address_v4, is_multicast)
{
  addr_t a;
  EXPECT_FALSE(a.is_multicast());

  addr_t b{multicast};
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
  EXPECT_EQ("1.2.3.4", addr_t{bytes}.to_string());
  EXPECT_EQ("224.1.2.3", addr_t{multicast}.to_string());
}


TEST_F(net_ip_address_v4, hash)
{
  EXPECT_EQ(addr_t::any().hash(), addr_t::any().hash());
  EXPECT_NE(addr_t::any().hash(), addr_t::loopback().hash());
}


TEST_F(net_ip_address_v4, memory_writer_inserter)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + INET_ADDRSTRLEN};

  writer << addr_t::any();
  EXPECT_STREQ("0.0.0.0", data);

  writer.first = data;
  writer << addr_t::loopback();
  EXPECT_STREQ("127.0.0.1", data);

  writer.first = data;
  writer << addr_t::broadcast();
  EXPECT_STREQ("255.255.255.255", data);

  writer.first = data;
  writer << addr_t{bytes};
  EXPECT_STREQ("1.2.3.4", data);

  writer.first = data;
  writer << addr_t{multicast};
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
  oss << addr_t{bytes};
  EXPECT_EQ("1.2.3.4", oss.str());

  oss.str("");
  oss << addr_t{multicast};
  EXPECT_EQ("224.1.2.3", oss.str());
}


TEST_F(net_ip_address_v4, comparisons)
{
  auto a = addr_t::any();
  auto b = addr_t::broadcast();
  auto c = a;

  EXPECT_FALSE(a == b);
  EXPECT_FALSE(b == a);
  EXPECT_TRUE(a == c);

  EXPECT_TRUE(a != b);
  EXPECT_TRUE(b != a);
  EXPECT_FALSE(a != c);

  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
  EXPECT_FALSE(a < c);

  EXPECT_FALSE(a > b);
  EXPECT_TRUE(b > a);
  EXPECT_FALSE(a > c);

  EXPECT_TRUE(a <= b);
  EXPECT_FALSE(b <= a);
  EXPECT_TRUE(a <= c);

  EXPECT_FALSE(a >= b);
  EXPECT_TRUE(b >= a);
  EXPECT_TRUE(a >= c);
}


TEST_F(net_ip_address_v4, make_address_bytes)
{
  auto a = sal::net::ip::make_address_v4(addr_t::loopback().to_bytes());
  EXPECT_EQ(addr_t::loopback(), a);
}


TEST_F(net_ip_address_v4, make_address_uint)
{
  auto a = sal::net::ip::make_address_v4(addr_t::loopback().to_uint());
  EXPECT_EQ(addr_t::loopback(), a);
}


TEST_F(net_ip_address_v4, make_address_cstr)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address_v4("127.0.0.1", ec);
  EXPECT_EQ(addr_t::loopback(), a);
  EXPECT_FALSE(bool(ec));
}


TEST_F(net_ip_address_v4, make_address_cstr_invalid)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address_v4(case_name.c_str(), ec);
  EXPECT_EQ(addr_t::any(), a);
  EXPECT_TRUE(bool(ec));
}


TEST_F(net_ip_address_v4, make_address_string)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address_v4(std::string("127.0.0.1"), ec);
  EXPECT_EQ(addr_t::loopback(), a);
  EXPECT_FALSE(bool(ec));
}


TEST_F(net_ip_address_v4, make_address_string_invalid)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address_v4(case_name, ec);
  EXPECT_EQ(addr_t::any(), a);
  EXPECT_TRUE(bool(ec));
}


TEST_F(net_ip_address_v4, make_address_cstr_throw)
{
  auto a = sal::net::ip::make_address_v4("127.0.0.1");
  EXPECT_EQ(addr_t::loopback(), a);
}


TEST_F(net_ip_address_v4, make_address_cstr_invalid_throw)
{
  EXPECT_THROW(
    sal::net::ip::make_address_v4(case_name.c_str()),
    std::system_error
  );
}


TEST_F(net_ip_address_v4, make_address_string_throw)
{
  auto a = sal::net::ip::make_address_v4(std::string("127.0.0.1"));
  EXPECT_EQ(addr_t::loopback(), a);
}


TEST_F(net_ip_address_v4, make_address_string_invalid_throw)
{
  EXPECT_THROW(
    sal::net::ip::make_address_v4(case_name),
    std::system_error
  );
}


} // namespace
