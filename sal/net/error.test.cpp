#include <sal/net/error.hpp>
#include <sal/common.test.hpp>


namespace {


using net_resolver_error = sal_test::with_value<sal::net::ip::resolver_errc_t>;


TEST_P(net_resolver_error, make_error_code)
{
  auto error = sal::net::ip::make_error_code(GetParam());

  EXPECT_TRUE(bool(error));
  EXPECT_NE(0, error.value());
  EXPECT_EQ(sal::net::ip::resolver_category(), error.category());
  EXPECT_FALSE(error.message().empty());
  EXPECT_STREQ("resolver", error.category().name());
}


TEST_F(net_resolver_error, make_error_code_invalid)
{
  auto error = sal::net::ip::make_error_code(
    static_cast<sal::net::ip::resolver_errc_t>(-1)
  );

  EXPECT_TRUE(bool(error));
  EXPECT_NE(0, error.value());
  EXPECT_EQ(sal::net::ip::resolver_category(), error.category());
  EXPECT_FALSE(error.message().empty());
  EXPECT_STREQ("resolver", error.category().name());
}


INSTANTIATE_TEST_CASE_P(net_resolver_error, net_resolver_error,
  ::testing::Values(
    sal::net::ip::resolver_errc_t::host_not_found,
    sal::net::ip::resolver_errc_t::host_not_found_try_again,
    sal::net::ip::resolver_errc_t::service_not_found
  )
);


using net_socket_error = sal_test::with_value<sal::net::socket_errc_t>;


TEST_P(net_socket_error, make_error_code)
{
  auto error = sal::net::make_error_code(GetParam());

  EXPECT_TRUE(bool(error));
  EXPECT_NE(0, error.value());
  EXPECT_EQ(sal::net::socket_category(), error.category());
  EXPECT_FALSE(error.message().empty());
  EXPECT_STREQ("socket", error.category().name());
}


TEST_F(net_socket_error, make_error_code_invalid)
{
  auto error = sal::net::make_error_code(
    static_cast<sal::net::socket_errc_t>(-1)
  );

  EXPECT_TRUE(bool(error));
  EXPECT_NE(0, error.value());
  EXPECT_EQ(sal::net::socket_category(), error.category());
  EXPECT_FALSE(error.message().empty());
  EXPECT_STREQ("socket", error.category().name());
}


INSTANTIATE_TEST_CASE_P(net_socket_error, net_socket_error,
  ::testing::Values(
    sal::net::socket_errc_t::already_open,
    sal::net::socket_errc_t::already_associated
  )
);


} // namespace
