#include <sal/net/ip/address.hpp>
#include <sal/common.test.hpp>


namespace {


struct net_ip_address
  : public sal_test::fixture
{
  using addr_t = sal::net::ip::address_t;
  using addr_v4_t = sal::net::ip::address_v4_t;
  using addr_v6_t = sal::net::ip::address_v6_t;

  addr_v4_t::bytes_t multicast_v4 =
  {
    { 224, 1, 2, 3 }
  };

  addr_v6_t::bytes_t multicast_v6 =
  {
    { 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }
  };
};


TEST_F(net_ip_address, ctor)
{
  addr_t a;
  EXPECT_TRUE(a.is_v4());
  EXPECT_FALSE(a.is_v6());
  EXPECT_TRUE(a.is_unspecified());
}


TEST_F(net_ip_address, ctor_v4_any)
{
  addr_t a{addr_v4_t::any};
  EXPECT_TRUE(a.is_v4());
  EXPECT_FALSE(a.is_v6());
  EXPECT_TRUE(a.is_unspecified());
}


TEST_F(net_ip_address, ctor_v6_any)
{
  addr_t a{addr_v6_t::any};
  EXPECT_TRUE(a.is_v6());
  EXPECT_FALSE(a.is_v4());
  EXPECT_TRUE(a.is_unspecified());
}


TEST_F(net_ip_address, ctor_v4_loopback)
{
  addr_t a{addr_v4_t::loopback};
  EXPECT_TRUE(a.is_v4());
  EXPECT_FALSE(a.is_v6());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, ctor_v6_loopback)
{
  addr_t a{addr_v6_t::loopback};
  EXPECT_TRUE(a.is_v6());
  EXPECT_FALSE(a.is_v4());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, ctor_v4_multicast)
{
  addr_t a{multicast_v4};
  EXPECT_TRUE(a.is_v4());
  EXPECT_FALSE(a.is_v6());
  EXPECT_TRUE(a.is_multicast());
}


TEST_F(net_ip_address, ctor_v6_multicast)
{
  addr_t a{multicast_v6};
  EXPECT_TRUE(a.is_v6());
  EXPECT_FALSE(a.is_v4());
  EXPECT_TRUE(a.is_multicast());
}


TEST_F(net_ip_address, ctor_sockaddr_v4)
{
  sockaddr_storage buf;
  auto &a{reinterpret_cast<sockaddr_in &>(buf)};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  addr_t addr{buf};
  ASSERT_TRUE(addr.is_v4());
  EXPECT_EQ(addr_v4_t::loopback, addr.to_v4());
}


TEST_F(net_ip_address, ctor_sockaddr_v6)
{
  sockaddr_storage buf;
  auto &a{reinterpret_cast<sockaddr_in6 &>(buf)};
  a.sin6_family = AF_INET6;
  a.sin6_addr = IN6ADDR_LOOPBACK_INIT;

  addr_t addr{buf};
  ASSERT_TRUE(addr.is_v6());
  EXPECT_EQ(addr_v6_t::loopback, addr.to_v6());
}


TEST_F(net_ip_address, ctor_sockaddr_invalid)
{
  sockaddr_storage a;
  a.ss_family = AF_UNIX;
  EXPECT_THROW(addr_t{a}, sal::net::ip::bad_address_cast_t);
}


TEST_F(net_ip_address, try_load_v4)
{
  sockaddr_storage buf;
  auto &a{reinterpret_cast<sockaddr_in &>(buf)};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  addr_t addr;
  ASSERT_TRUE(addr.try_load(buf));
  EXPECT_TRUE(addr.is_v4());
  EXPECT_EQ(addr_v4_t::loopback, addr.to_v4());
}


TEST_F(net_ip_address, try_load_v6)
{
  sockaddr_storage buf;
  auto &a{reinterpret_cast<sockaddr_in6 &>(buf)};
  a.sin6_family = AF_INET6;
  a.sin6_addr = IN6ADDR_LOOPBACK_INIT;

  addr_t addr;
  ASSERT_TRUE(addr.try_load(buf));
  EXPECT_TRUE(addr.is_v6());
  EXPECT_EQ(addr_v6_t::loopback, addr.to_v6());
}


TEST_F(net_ip_address, try_load_invalid)
{
  sockaddr_storage a;
  a.ss_family = AF_INET + AF_INET6;
  addr_t addr;
  EXPECT_FALSE(addr.try_load(a));
}


TEST_F(net_ip_address, load_v4)
{
  sockaddr_storage buf;
  auto &a{reinterpret_cast<sockaddr_in &>(buf)};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  addr_t addr;
  addr.load(buf);
  ASSERT_TRUE(addr.is_v4());
  EXPECT_EQ(addr_v4_t::loopback, addr.to_v4());
}


TEST_F(net_ip_address, load_v6)
{
  sockaddr_storage buf;
  auto &a{reinterpret_cast<sockaddr_in6 &>(buf)};
  a.sin6_family = AF_INET6;
  a.sin6_addr = IN6ADDR_LOOPBACK_INIT;

  addr_t addr;
  addr.load(buf);
  ASSERT_TRUE(addr.is_v6());
  EXPECT_EQ(addr_v6_t::loopback, addr.to_v6());
}


TEST_F(net_ip_address, load_invalid)
{
  sockaddr_storage a;
  a.ss_family = AF_UNIX;
  addr_t addr;
  EXPECT_THROW(addr.load(a), sal::net::ip::bad_address_cast_t);
}


TEST_F(net_ip_address, store_v4)
{
  addr_t addr{addr_v4_t::loopback};

  sockaddr_storage buf;
  addr.store(buf);
  auto &a{reinterpret_cast<sockaddr_in &>(buf)};

  ASSERT_EQ(AF_INET, a.sin_family);
  EXPECT_EQ(static_cast<uint32_t>(INADDR_LOOPBACK), ntohl(a.sin_addr.s_addr));
}


TEST_F(net_ip_address, store_v6)
{
  addr_t addr{addr_v6_t::loopback};

  sockaddr_storage buf;
  addr.store(buf);
  auto &a{reinterpret_cast<sockaddr_in6 &>(buf)};

  ASSERT_EQ(AF_INET6, a.sin6_family);
  EXPECT_TRUE(IN6_IS_ADDR_LOOPBACK(&a.sin6_addr) != 0);
}


TEST_F(net_ip_address, assign)
{
  addr_t a, b{addr_v4_t{}};
  a = b;
  EXPECT_TRUE(a.is_v4());
  EXPECT_FALSE(a.is_v6());
  EXPECT_TRUE(a.is_unspecified());
}


TEST_F(net_ip_address, assign_v4_any)
{
  addr_t a, b{addr_v4_t::any};
  a = b;
  EXPECT_TRUE(a.is_v4());
  EXPECT_FALSE(a.is_v6());
  EXPECT_TRUE(a.is_unspecified());
}


TEST_F(net_ip_address, assign_v6_any)
{
  addr_t a, b{addr_v6_t::any};
  a = b;
  EXPECT_TRUE(a.is_v6());
  EXPECT_FALSE(a.is_v4());
  EXPECT_TRUE(a.is_unspecified());
}


TEST_F(net_ip_address, assign_v4_loopback)
{
  addr_t a, b{addr_v4_t::loopback};
  a = b;
  EXPECT_TRUE(a.is_v4());
  EXPECT_FALSE(a.is_v6());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, assign_v6_loopback)
{
  addr_t a, b{addr_v6_t::loopback};
  a = b;
  EXPECT_TRUE(a.is_v6());
  EXPECT_FALSE(a.is_v4());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, assign_v4_multicast)
{
  addr_t a, b{multicast_v4};
  a = b;
  EXPECT_TRUE(a.is_v4());
  EXPECT_FALSE(a.is_v6());
  EXPECT_TRUE(a.is_multicast());
}


TEST_F(net_ip_address, assign_v6_multicast)
{
  addr_t a, b{multicast_v6};
  a = b;
  EXPECT_TRUE(a.is_v6());
  EXPECT_FALSE(a.is_v4());
  EXPECT_TRUE(a.is_multicast());
}


TEST_F(net_ip_address, as_v4)
{
  addr_t a{multicast_v4};
  ASSERT_NE(nullptr, a.as_v4());
  EXPECT_TRUE(a.as_v4()->is_multicast());
}


TEST_F(net_ip_address, as_v4_invalid)
{
  addr_t a{multicast_v6};
  EXPECT_EQ(nullptr, a.as_v4());
}


TEST_F(net_ip_address, as_v6)
{
  addr_t a{multicast_v6};
  ASSERT_NE(nullptr, a.as_v6());
  EXPECT_TRUE(a.as_v6()->is_multicast());
}


TEST_F(net_ip_address, as_v6_invalid)
{
  addr_t a{multicast_v4};
  EXPECT_EQ(nullptr, a.as_v6());
}


TEST_F(net_ip_address, to_v4)
{
  addr_t a{multicast_v4};
  ASSERT_NO_THROW(a.to_v4());
  EXPECT_TRUE(a.to_v4().is_multicast());
}


TEST_F(net_ip_address, to_v4_invalid)
{
  addr_t a{multicast_v6};
  EXPECT_THROW(a.to_v4(), sal::net::ip::bad_address_cast_t);
}


TEST_F(net_ip_address, to_v6)
{
  addr_t a{multicast_v6};
  ASSERT_NO_THROW(a.to_v6());
  EXPECT_TRUE(a.to_v6().is_multicast());
}


TEST_F(net_ip_address, to_v6_invalid)
{
  addr_t a{multicast_v4};
  EXPECT_THROW(a.to_v6(), sal::net::ip::bad_address_cast_t);
}


TEST_F(net_ip_address, to_string_v4)
{
  EXPECT_EQ("224.1.2.3", addr_t{multicast_v4}.to_string());
}


TEST_F(net_ip_address, to_string_v6)
{
  EXPECT_EQ("ff00::1", addr_t{multicast_v6}.to_string());
}


TEST_F(net_ip_address, hash_v4)
{
  EXPECT_EQ(
    addr_t{addr_v4_t::any}.hash(),
    addr_t{addr_v4_t::any}.hash()
  );
  EXPECT_NE(
    addr_t{addr_v4_t::any}.hash(),
    addr_t{addr_v4_t::loopback}.hash()
  );
}


TEST_F(net_ip_address, hash_v6)
{
  EXPECT_EQ(
    addr_t{addr_v6_t::any}.hash(),
    addr_t{addr_v6_t::any}.hash()
  );
  EXPECT_NE(
    addr_t{addr_v6_t::any}.hash(),
    addr_t{addr_v6_t::loopback}.hash()
  );
}


TEST_F(net_ip_address, hash_v4_v6)
{
  EXPECT_NE(
    addr_t{addr_v4_t::any}.hash(),
    addr_t{addr_v6_t::any}.hash()
  );
  EXPECT_NE(
    addr_t{addr_v4_t::any}.hash(),
    addr_t{addr_v6_t::loopback}.hash()
  );
}


TEST_F(net_ip_address, memory_writer_inserter_v4)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + INET_ADDRSTRLEN};
  writer << addr_t{multicast_v4};
  EXPECT_STREQ("224.1.2.3", data);
}


TEST_F(net_ip_address, memory_writer_inserter_v6)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + INET6_ADDRSTRLEN};
  writer << addr_t{multicast_v6};
  EXPECT_STREQ("ff00::1", data);
}


TEST_F(net_ip_address, memory_writer_inserter_v4_exact)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof("0.0.0.0")};
  EXPECT_TRUE(bool(writer << addr_t{addr_v4_t::any}));
  EXPECT_STREQ("0.0.0.0", data);
}


TEST_F(net_ip_address, memory_writer_inserter_v6_exact)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof("::")};
  EXPECT_TRUE(bool(writer << addr_t{addr_v6_t::any}));
  EXPECT_STREQ("::", data);
}


TEST_F(net_ip_address, memory_writer_inserter_v4_overflow)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof(".")};
  EXPECT_FALSE(bool(writer << addr_t{multicast_v4}));
}


TEST_F(net_ip_address, memory_writer_inserter_v6_overflow)
{
  char data[1024];
  sal::memory_writer_t writer{data, data + sizeof(".")};
  EXPECT_FALSE(bool(writer << addr_t{multicast_v6}));
}


TEST_F(net_ip_address, ostream_inserter_v4)
{
  std::ostringstream oss;
  oss << addr_t{multicast_v4};
  EXPECT_EQ("224.1.2.3", oss.str());
}


TEST_F(net_ip_address, ostream_inserter_v6)
{
  std::ostringstream oss;
  oss << addr_t{multicast_v6};
  EXPECT_EQ("ff00::1", oss.str());
}


TEST_F(net_ip_address, comparisons_v4)
{
  auto a = addr_t{addr_v4_t::any};
  auto b = addr_t{multicast_v4};
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


TEST_F(net_ip_address, comparisons_v6)
{
  auto a = addr_t{addr_v6_t::any};
  auto b = addr_t{multicast_v6};
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


TEST_F(net_ip_address, comparisons_v4_v6)
{
  auto a = addr_t{addr_v4_t::any};
  auto b = addr_t{multicast_v6};
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


TEST_F(net_ip_address, comparisons_v6_v4)
{
  auto a = addr_t{addr_v6_t::any};
  auto b = addr_t{multicast_v4};
  auto c = a;

  EXPECT_FALSE(a == b);
  EXPECT_FALSE(b == a);
  EXPECT_TRUE(a == c);

  EXPECT_TRUE(a != b);
  EXPECT_TRUE(b != a);
  EXPECT_FALSE(a != c);

  EXPECT_FALSE(a < b);
  EXPECT_TRUE(b < a);
  EXPECT_FALSE(a < c);

  EXPECT_TRUE(a > b);
  EXPECT_FALSE(b > a);
  EXPECT_FALSE(a > c);

  EXPECT_FALSE(a <= b);
  EXPECT_TRUE(b <= a);
  EXPECT_TRUE(a <= c);

  EXPECT_TRUE(a >= b);
  EXPECT_FALSE(b >= a);
  EXPECT_TRUE(a >= c);
}


TEST_F(net_ip_address, make_address_cstr_v4)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address("127.0.0.1", ec);
  ASSERT_FALSE(bool(ec));
  EXPECT_TRUE(a.is_v4());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, make_address_cstr_v6)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address("::1", ec);
  ASSERT_FALSE(bool(ec));
  EXPECT_TRUE(a.is_v6());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, make_address_cstr_no_error_v4)
{
  auto a = sal::net::ip::make_address("127.0.0.1");
  EXPECT_TRUE(a.is_v4());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, make_address_cstr_no_error_v6)
{
  auto a = sal::net::ip::make_address("::1");
  EXPECT_TRUE(a.is_v6());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, make_address_cstr_invalid)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address(case_name.c_str(), ec);
  EXPECT_TRUE(bool(ec));
  EXPECT_TRUE(a.is_unspecified());
}


TEST_F(net_ip_address, make_address_cstr_invalid_throw)
{
  EXPECT_THROW(
    sal::net::ip::make_address(case_name.c_str()),
    std::system_error
  );
}


TEST_F(net_ip_address, make_address_string_v4)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address(std::string("127.0.0.1"), ec);
  ASSERT_FALSE(bool(ec));
  EXPECT_TRUE(a.is_v4());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, make_address_string_v6)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address(std::string("::1"), ec);
  ASSERT_FALSE(bool(ec));
  EXPECT_TRUE(a.is_v6());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, make_address_string_no_error_v4)
{
  auto a = sal::net::ip::make_address(std::string("127.0.0.1"));
  EXPECT_TRUE(a.is_v4());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, make_address_string_no_error_v6)
{
  auto a = sal::net::ip::make_address(std::string("::1"));
  EXPECT_TRUE(a.is_v6());
  EXPECT_TRUE(a.is_loopback());
}


TEST_F(net_ip_address, make_address_string_invalid)
{
  std::error_code ec{};
  auto a = sal::net::ip::make_address(case_name, ec);
  EXPECT_TRUE(bool(ec));
  EXPECT_TRUE(a.is_unspecified());
}


TEST_F(net_ip_address, make_address_string_invalid_throw)
{
  EXPECT_THROW(
    sal::net::ip::make_address(case_name),
    std::system_error
  );
}


} // namespace
