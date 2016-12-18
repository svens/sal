#include <sal/net/ip/address_v6.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_ip_address_v6
  : public sal_test::fixture
{
  using addr_t = sal::net::ip::address_v6_t;

  addr_t::scope_id_t scope{1};

  addr_t::bytes_t bytes =
  {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
  };

  addr_t::bytes_t link_local =
  {
    { 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
  };

  addr_t::bytes_t site_local =
  {
    { 0xfe, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
  };

  addr_t::bytes_t v4_mapped =
  {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 }
  };

  addr_t::bytes_t multicast =
  {
    { 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
  };

  addr_t::bytes_t multicast_node_local =
  {
    { 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
  };

  addr_t::bytes_t multicast_link_local =
  {
    { 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
  };

  addr_t::bytes_t multicast_site_local =
  {
    { 0xff, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
  };

  addr_t::bytes_t multicast_org_local =
  {
    { 0xff, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
  };

  addr_t::bytes_t multicast_global =
  {
    { 0xff, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
  };
};


TEST_F(net_ip_address_v6, ctor)
{
  addr_t a;
  EXPECT_TRUE(a.is_unspecified());
  EXPECT_EQ(0U, a.scope_id());
}


TEST_F(net_ip_address_v6, ctor_bytes)
{
  addr_t a{bytes, scope};
  EXPECT_EQ(bytes, a.to_bytes());
  EXPECT_EQ(scope, a.scope_id());
}


TEST_F(net_ip_address_v6, ctor_address_v6)
{
  addr_t a{bytes, scope}, b{a};
  EXPECT_EQ(bytes, b.to_bytes());
  EXPECT_EQ(scope, b.scope_id());
}


TEST_F(net_ip_address_v6, operator_assign)
{
  addr_t a{bytes}, b;
  b = a;
  EXPECT_EQ(bytes, b.to_bytes());
}


TEST_F(net_ip_address_v6, scope)
{
  addr_t a{bytes};
  EXPECT_EQ(0U, a.scope_id());

  a.scope_id(scope);
  EXPECT_EQ(scope, a.scope_id());
}


TEST_F(net_ip_address_v6, is_unspecified)
{
  addr_t a;
  EXPECT_TRUE(a.is_unspecified());

  addr_t b{bytes};
  EXPECT_FALSE(b.is_unspecified());

  EXPECT_TRUE(addr_t::any().is_unspecified());
  EXPECT_FALSE(addr_t::loopback().is_unspecified());
}


TEST_F(net_ip_address_v6, is_loopback)
{
  addr_t a;
  EXPECT_FALSE(a.is_loopback());

  addr_t b{bytes};
  EXPECT_FALSE(b.is_loopback());

  EXPECT_FALSE(addr_t::any().is_loopback());
  EXPECT_TRUE(addr_t::loopback().is_loopback());
}


TEST_F(net_ip_address_v6, is_link_local)
{
  EXPECT_TRUE(addr_t{link_local}.is_link_local());
  EXPECT_FALSE(addr_t::any().is_link_local());
  EXPECT_FALSE(addr_t::loopback().is_link_local());
}


TEST_F(net_ip_address_v6, is_site_local)
{
  EXPECT_TRUE(addr_t{site_local}.is_site_local());
  EXPECT_FALSE(addr_t::any().is_site_local());
  EXPECT_FALSE(addr_t::loopback().is_site_local());
}


TEST_F(net_ip_address_v6, is_v4_mapped)
{
  EXPECT_TRUE(addr_t{v4_mapped}.is_v4_mapped());
  EXPECT_FALSE(addr_t::any().is_v4_mapped());
  EXPECT_FALSE(addr_t::loopback().is_v4_mapped());
}


TEST_F(net_ip_address_v6, is_multicast)
{
  EXPECT_TRUE(addr_t{multicast}.is_multicast());
  EXPECT_FALSE(addr_t::any().is_multicast());
  EXPECT_FALSE(addr_t::loopback().is_multicast());
}


TEST_F(net_ip_address_v6, is_multicast_node_local)
{
  EXPECT_TRUE(addr_t{multicast_node_local}.is_multicast_node_local());
  EXPECT_FALSE(addr_t::any().is_multicast_node_local());
  EXPECT_FALSE(addr_t::loopback().is_multicast_node_local());
}


TEST_F(net_ip_address_v6, is_multicast_link_local)
{
  EXPECT_TRUE(addr_t{multicast_link_local}.is_multicast_link_local());
  EXPECT_FALSE(addr_t::any().is_multicast_link_local());
  EXPECT_FALSE(addr_t::loopback().is_multicast_link_local());
}


TEST_F(net_ip_address_v6, is_multicast_site_local)
{
  EXPECT_TRUE(addr_t{multicast_site_local}.is_multicast_site_local());
  EXPECT_FALSE(addr_t::any().is_multicast_site_local());
  EXPECT_FALSE(addr_t::loopback().is_multicast_site_local());
}


TEST_F(net_ip_address_v6, is_multicast_org_local)
{
  EXPECT_TRUE(addr_t{multicast_org_local}.is_multicast_org_local());
  EXPECT_FALSE(addr_t::any().is_multicast_org_local());
  EXPECT_FALSE(addr_t::loopback().is_multicast_org_local());
}


TEST_F(net_ip_address_v6, is_multicast_global)
{
  EXPECT_TRUE(addr_t{multicast_global}.is_multicast_global());
  EXPECT_FALSE(addr_t::any().is_multicast_global());
  EXPECT_FALSE(addr_t::loopback().is_multicast_global());
}


TEST_F(net_ip_address_v6, to_string)
{
  EXPECT_EQ("::", addr_t::any().to_string());
  EXPECT_EQ("::1", addr_t::loopback().to_string());
  EXPECT_EQ("ff00::1", addr_t{multicast}.to_string());
}


TEST_F(net_ip_address_v6, hash)
{
  EXPECT_EQ(addr_t::any().hash(), addr_t::any().hash());
  EXPECT_NE(addr_t::any().hash(), addr_t::loopback().hash());
}


TEST_F(net_ip_address_v6, memory_writer_inserter)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + INET6_ADDRSTRLEN};

  writer << addr_t::any();
  EXPECT_STREQ("::", data);

  writer.first = data;
  writer << addr_t::loopback();
  EXPECT_STREQ("::1", data);

  writer.first = data;
  writer << addr_t{multicast};
  EXPECT_STREQ("ff00::1", data);
}


TEST_F(net_ip_address_v6, memory_writer_inserter_exact)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof("::")};
  EXPECT_TRUE(bool(writer << addr_t::any()));
  EXPECT_STREQ("::", data);
}


TEST_F(net_ip_address_v6, memory_writer_inserter_overflow)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof("..")};
  EXPECT_FALSE(bool(writer << addr_t{multicast}));
}


TEST_F(net_ip_address_v6, ostream_inserter)
{
  std::ostringstream oss;

  oss << addr_t::any();
  EXPECT_EQ("::", oss.str());

  oss.str("");
  oss << addr_t::loopback();
  EXPECT_EQ("::1", oss.str());

  oss.str("");
  oss << addr_t{multicast};
  EXPECT_EQ("ff00::1", oss.str());
}


TEST_F(net_ip_address_v6, comparisons)
{
  auto a = addr_t::any();
  auto b = addr_t::loopback();
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


TEST_F(net_ip_address_v6, make_address_bytes)
{
  auto a = sal::net::ip::make_address_v6(addr_t::loopback().to_bytes());
  EXPECT_EQ(addr_t::loopback(), a);
}


TEST_F(net_ip_address_v6, make_address_cstr)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address_v6("::1", ec);
  EXPECT_EQ(addr_t::loopback(), a);
  EXPECT_FALSE(bool(ec));
}


TEST_F(net_ip_address_v6, make_address_cstr_invalid)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address_v6(case_name.c_str(), ec);
  EXPECT_EQ(addr_t::any(), a);
  EXPECT_TRUE(bool(ec));
}


TEST_F(net_ip_address_v6, make_address_string)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address_v6(std::string("::1"), ec);
  EXPECT_EQ(addr_t::loopback(), a);
  EXPECT_FALSE(bool(ec));
}


TEST_F(net_ip_address_v6, make_address_string_invalid)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address_v6(case_name, ec);
  EXPECT_EQ(addr_t::any(), a);
  EXPECT_TRUE(bool(ec));
}


TEST_F(net_ip_address_v6, make_address_cstr_throw)
{
  auto a = sal::net::ip::make_address_v6("::1");
  EXPECT_EQ(addr_t::loopback(), a);
}


TEST_F(net_ip_address_v6, make_address_cstr_invalid_throw)
{
  EXPECT_THROW(
    sal::net::ip::make_address_v6(case_name.c_str()),
    std::system_error
  );
}


TEST_F(net_ip_address_v6, make_address_string_throw)
{
  auto a = sal::net::ip::make_address_v6(std::string("::1"));
  EXPECT_EQ(addr_t::loopback(), a);
}


TEST_F(net_ip_address_v6, make_address_string_invalid_throw)
{
  EXPECT_THROW(
    sal::net::ip::make_address_v6(case_name),
    std::system_error
  );
}


TEST_F(net_ip_address_v6, make_address_v4_mapped)
{
  auto a4 = sal::net::ip::address_v4_t::loopback();
  auto a6 = sal::net::ip::make_address_v6(a4);
  auto b = sal::net::ip::make_address_v4(a6);
  EXPECT_EQ(a4, b);
}


TEST_F(net_ip_address_v6, make_address_v4_mapped_invalid)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address_v4(addr_t::any(), ec);
  EXPECT_TRUE(a.is_unspecified());
  EXPECT_TRUE(bool(ec));
}


TEST_F(net_ip_address_v6, make_address_v4_mapped_invalid_throw)
{
  EXPECT_THROW(
    sal::net::ip::make_address_v4(addr_t::any()),
    sal::net::ip::bad_address_cast_t
  );
}


} // namespace
