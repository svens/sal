#include <sal/net/ip/resolver.hpp>
#include <sal/net/ip/tcp.hpp>
#include <sal/net/ip/udp.hpp>
#include <sal/common.test.hpp>


namespace {


template <typename Protocol>
struct net_ip_resolver
  : public sal_test::with_type<Protocol>
{
};

using protocol_types = testing::Types<
  sal::net::ip::tcp_t,
  sal::net::ip::udp_t
>;
TYPED_TEST_CASE(net_ip_resolver, protocol_types);


TYPED_TEST(net_ip_resolver, entry_ctor)
{
  using endpoint_t = typename TypeParam::endpoint_t;
  sal::net::ip::basic_resolver_entry_t<TypeParam> entry;
  EXPECT_EQ(endpoint_t(), entry.endpoint());
  EXPECT_EQ(endpoint_t(), endpoint_t(entry));
  EXPECT_EQ(nullptr, entry.host_name());
  EXPECT_EQ(nullptr, entry.service_name());
}


TYPED_TEST(net_ip_resolver, iterator_star)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("localhost", "echo");
  ASSERT_FALSE(results.empty()) << "can't test without resolver results";

  auto it = results.begin();
  EXPECT_EQ(7U, (*it).endpoint().port());
}


TYPED_TEST(net_ip_resolver, iterator_arrow)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("localhost", "echo");
  ASSERT_FALSE(results.empty()) << "can't test without resolver results";

  auto it = results.begin();
  EXPECT_EQ(7U, it->endpoint().port());
}


TYPED_TEST(net_ip_resolver, iterator_pre_inc)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("localhost", "echo");
  ASSERT_FALSE(results.empty()) << "can't test without resolver results";

  auto a = results.begin();
  EXPECT_EQ(7U, a->endpoint().port());

  auto b = a;
  auto c = ++a;
  EXPECT_TRUE(b != c);
  EXPECT_TRUE(c == a);
}


TYPED_TEST(net_ip_resolver, iterator_post_inc)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("localhost", "echo");
  ASSERT_FALSE(results.empty()) << "can't test without resolver results";

  auto a = results.begin();
  EXPECT_EQ(7U, a->endpoint().port());

  auto b = a;
  auto c = a++;
  EXPECT_TRUE(b == c);
  EXPECT_TRUE(c != a);
}


TYPED_TEST(net_ip_resolver, results_ctor)
{
  sal::net::ip::basic_resolver_results_t<TypeParam> results;
  EXPECT_TRUE(results.empty());
  EXPECT_EQ(0U, results.size());
  EXPECT_EQ("", results.host_name());
  EXPECT_EQ("", results.service_name());
  EXPECT_EQ(results.end(), results.begin());
  EXPECT_TRUE(results.begin() == results.end());
  EXPECT_FALSE(results.begin() != results.end());
  EXPECT_TRUE(results.cbegin() == results.cend());
  EXPECT_FALSE(results.cbegin() != results.cend());
}


TYPED_TEST(net_ip_resolver, results_move_ctor)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("localhost", "0");
  EXPECT_FALSE(results.empty());

  auto a(std::move(results));
  EXPECT_FALSE(a.empty());
  EXPECT_TRUE(results.empty());
}


TYPED_TEST(net_ip_resolver, results_move_assign)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("localhost", "0");
  EXPECT_FALSE(results.empty());

  sal::net::ip::basic_resolver_results_t<TypeParam> a;
  EXPECT_TRUE(a.empty());
  a = std::move(results);
  EXPECT_FALSE(a.empty());
  EXPECT_TRUE(results.empty());
}


TYPED_TEST(net_ip_resolver, results_swap)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("localhost", "0");
  EXPECT_FALSE(results.empty());

  sal::net::ip::basic_resolver_results_t<TypeParam> a;
  EXPECT_TRUE(a.empty());

  results.swap(a);
  EXPECT_FALSE(a.empty());
  EXPECT_TRUE(results.empty());
}


TYPED_TEST(net_ip_resolver, resolve_host_localhost)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("localhost", nullptr);
  EXPECT_FALSE(results.empty());
  EXPECT_EQ("localhost", results.host_name());
  EXPECT_TRUE(results.service_name().empty());

  for (auto &a: results)
  {
    EXPECT_TRUE(a.endpoint().address().is_loopback());
  }
}


TYPED_TEST(net_ip_resolver, resolve_service_echo)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve(nullptr, "echo");
  EXPECT_FALSE(results.empty());
  EXPECT_TRUE(results.host_name().empty());
  EXPECT_EQ("echo", results.service_name());

  for (auto &a: results)
  {
    EXPECT_EQ(7U, a.endpoint().port());
  }
}


TYPED_TEST(net_ip_resolver, resolve_host_invalid)
{
  typename TypeParam::resolver_t resolver;
  std::error_code error;
  auto results = resolver.resolve(sal_test::fixture::case_name.c_str(),
    nullptr,
    error
  );
  EXPECT_TRUE(results.empty());
  EXPECT_TRUE(bool(error));
  EXPECT_EQ(sal::net::ip::resolver_errc_t::host_not_found, error);
}


TYPED_TEST(net_ip_resolver, resolve_service_invalid)
{
  typename TypeParam::resolver_t resolver;
  std::error_code error;
  auto results = resolver.resolve(nullptr,
    sal_test::fixture::case_name.c_str(),
    error
  );
  EXPECT_TRUE(results.empty());
  EXPECT_TRUE(bool(error));
  EXPECT_EQ(sal::net::ip::resolver_errc_t::service_not_found, error);
}


TYPED_TEST(net_ip_resolver, resolve_host_invalid_throw)
{
  typename TypeParam::resolver_t resolver;
  EXPECT_THROW(
    resolver.resolve(sal_test::fixture::case_name.c_str(), nullptr),
    std::system_error
  );
}


TYPED_TEST(net_ip_resolver, resolve_service_invalid_throw)
{
  typename TypeParam::resolver_t resolver;
  EXPECT_THROW(
    resolver.resolve(nullptr, sal_test::fixture::case_name.c_str()),
    std::system_error
  );
}


TYPED_TEST(net_ip_resolver, resolve_passive_no_host)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve(nullptr, "echo", resolver.passive);

  EXPECT_FALSE(results.empty());
  EXPECT_TRUE(results.host_name().empty());
  EXPECT_EQ("echo", results.service_name());

  for (auto &a: results)
  {
    EXPECT_TRUE(a.endpoint().address().is_unspecified());
    EXPECT_EQ(7U, a.endpoint().port());
  }
}


TYPED_TEST(net_ip_resolver, resolve_passive_with_host)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("localhost", "echo", resolver.passive);

  EXPECT_FALSE(results.empty());
  EXPECT_EQ("localhost", results.host_name());
  EXPECT_EQ("echo", results.service_name());

  for (auto &a: results)
  {
    EXPECT_TRUE(a.endpoint().address().is_loopback());
    EXPECT_EQ(7U, a.endpoint().port());
  }
}


TYPED_TEST(net_ip_resolver, resolve_numeric_host_v4)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("127.0.0.1", nullptr, resolver.numeric_host);

  EXPECT_FALSE(results.empty());
  EXPECT_EQ("127.0.0.1", results.host_name());

  for (auto &a: results)
  {
    EXPECT_TRUE(a.endpoint().address().is_loopback());
  }
}


TYPED_TEST(net_ip_resolver, resolve_numeric_host_v6)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("::1", nullptr, resolver.numeric_host);

  EXPECT_FALSE(results.empty());
  EXPECT_EQ("::1", results.host_name());

  for (auto &a: results)
  {
    EXPECT_TRUE(a.endpoint().address().is_loopback());
  }
}


TYPED_TEST(net_ip_resolver, resolve_numeric_host_invalid)
{
  typename TypeParam::resolver_t resolver;
  EXPECT_THROW(
    resolver.resolve("localhost", nullptr, resolver.numeric_host),
    std::system_error
  );
}


TYPED_TEST(net_ip_resolver, resolve_numeric_service)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve(nullptr, "80", resolver.numeric_service);

  EXPECT_FALSE(results.empty());
  EXPECT_EQ("80", results.service_name());

  for (auto &a: results)
  {
    EXPECT_EQ(80U, a.endpoint().port());
  }
}


TYPED_TEST(net_ip_resolver, resolve_numeric_service_invalid)
{
  typename TypeParam::resolver_t resolver;
  EXPECT_THROW(
    resolver.resolve(nullptr, "http", resolver.numeric_service),
    std::system_error
  );
}


TYPED_TEST(net_ip_resolver, resolve_canonical_name)
{
  //
  // unfortunately this test depends on how mail.google.com has configured
  // once it starts failing, find another host whose canonical name is
  // different from host name
  //

  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve("mail.google.com",
    nullptr,
    resolver.canonical_name
  );

  EXPECT_FALSE(results.empty());
  for (auto &a: results)
  {
    EXPECT_STRNE("mail.google.com", a.host_name());
  }
}


TYPED_TEST(net_ip_resolver, resolve_v4_host_localhost)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve(TypeParam::v4(),
    "localhost",
    nullptr
  );
  EXPECT_FALSE(results.empty());
  EXPECT_EQ("localhost", results.host_name());
  EXPECT_TRUE(results.service_name().empty());

  for (auto &a: results)
  {
    EXPECT_TRUE(a.endpoint().address().is_v4());
    EXPECT_TRUE(a.endpoint().address().is_loopback());
  }
}


TYPED_TEST(net_ip_resolver, resolve_v6_host_localhost)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve(TypeParam::v6(),
    "localhost",
    nullptr
  );
  EXPECT_FALSE(results.empty());
  EXPECT_EQ("localhost", results.host_name());
  EXPECT_TRUE(results.service_name().empty());

  for (auto &a: results)
  {
    EXPECT_TRUE(a.endpoint().address().is_v6());
    EXPECT_TRUE(a.endpoint().address().is_loopback());
  }
}


TYPED_TEST(net_ip_resolver, resolve_v4_service_echo)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve(TypeParam::v4(), nullptr, "echo");
  EXPECT_FALSE(results.empty());
  EXPECT_TRUE(results.host_name().empty());
  EXPECT_EQ("echo", results.service_name());

  for (auto &a: results)
  {
    EXPECT_EQ(7U, a.endpoint().port());
  }
}


TYPED_TEST(net_ip_resolver, resolve_v6_service_echo)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve(TypeParam::v6(), nullptr, "echo");
  EXPECT_FALSE(results.empty());
  EXPECT_TRUE(results.host_name().empty());
  EXPECT_EQ("echo", results.service_name());

  for (auto &a: results)
  {
    EXPECT_EQ(7U, a.endpoint().port());
  }
}


TYPED_TEST(net_ip_resolver, resolve_v4_numeric_host_v4)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve(TypeParam::v4(),
    "127.0.0.1",
    nullptr,
    resolver.numeric_host
  );

  EXPECT_FALSE(results.empty());
  EXPECT_EQ("127.0.0.1", results.host_name());

  for (auto &a: results)
  {
    EXPECT_TRUE(a.endpoint().address().is_v4());
    EXPECT_TRUE(a.endpoint().address().is_loopback());
  }
}


TYPED_TEST(net_ip_resolver, resolve_v6_numeric_host_v6)
{
  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve(TypeParam::v6(),
    "::1",
    nullptr,
    resolver.numeric_host
  );

  EXPECT_FALSE(results.empty());
  EXPECT_EQ("::1", results.host_name());

  for (auto &a: results)
  {
    EXPECT_TRUE(a.endpoint().address().is_v6());
    EXPECT_TRUE(a.endpoint().address().is_loopback());
  }
}


TYPED_TEST(net_ip_resolver, resolve_v4_numeric_host_v6)
{
  typename TypeParam::resolver_t resolver;
  EXPECT_THROW(
    resolver.resolve(TypeParam::v4(),
      "::1",
      nullptr,
      resolver.numeric_host
    ),
    std::system_error
  );
}


TYPED_TEST(net_ip_resolver, resolve_v6_numeric_host_v4)
{
  typename TypeParam::resolver_t resolver;
  EXPECT_THROW(
    resolver.resolve(TypeParam::v6(),
      "127.0.0.1",
      nullptr,
      resolver.numeric_host
    ),
    std::system_error
  );
}


TYPED_TEST(net_ip_resolver, resolve_v4_canonical_name)
{
  // see also resolve_canonical_name

  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve(TypeParam::v4(),
    "mail.google.com",
    nullptr,
    resolver.canonical_name
  );

  EXPECT_FALSE(results.empty());
  for (auto &a: results)
  {
    EXPECT_STRNE("mail.google.com", a.host_name());
    EXPECT_TRUE(a.endpoint().address().is_v4());
  }
}


TYPED_TEST(net_ip_resolver, resolve_v6_canonical_name)
{
  // see also resolve_canonical_name

  typename TypeParam::resolver_t resolver;
  auto results = resolver.resolve(TypeParam::v6(),
    "mail.google.com",
    nullptr,
    resolver.canonical_name | resolver.v4_mapped
  );

  EXPECT_FALSE(results.empty());
  for (auto &a: results)
  {
    EXPECT_STRNE("mail.google.com", a.host_name());
    EXPECT_TRUE(a.endpoint().address().is_v6());
  }
}


TYPED_TEST(net_ip_resolver, resolve_v4_host_invalid)
{
  typename TypeParam::resolver_t resolver;
  std::error_code error;
  auto results = resolver.resolve(TypeParam::v4(),
    sal_test::fixture::case_name.c_str(),
    nullptr,
    error
  );
  EXPECT_TRUE(results.empty());
  EXPECT_TRUE(bool(error));
  EXPECT_EQ(sal::net::ip::resolver_errc_t::host_not_found, error);
}


TYPED_TEST(net_ip_resolver, resolve_v6_host_invalid)
{
  typename TypeParam::resolver_t resolver;
  std::error_code error;
  auto results = resolver.resolve(TypeParam::v6(),
    sal_test::fixture::case_name.c_str(),
    nullptr,
    error
  );
  EXPECT_TRUE(results.empty());
  EXPECT_TRUE(bool(error));
  EXPECT_EQ(sal::net::ip::resolver_errc_t::host_not_found, error);
}


} // namespace
